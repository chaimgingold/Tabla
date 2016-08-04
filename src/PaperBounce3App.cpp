#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/Rand.h"

#include "cinder/Text.h"
#include "cinder/gl/TextureFont.h"

#include "cinder/Capture.h"
#include "CinderOpenCV.h"

#include "Contour.h"
#include "BallWorld.h"
#include "geom.h"

#include <map>
#include <string>

using namespace ci;
using namespace ci::app;
using namespace std;


const float kResScale = .2f ;
const float kContourMinRadius = 3.f  * kResScale ;
const float kContourMinArea   = 100.f * kResScale ;
const float kContourDPEpislon = 5.f  * kResScale ;
const float kContourMinWidth  = 5.f  * kResScale ;


const vec2 kCaptureSize = vec2( 640, 480 ) ;
//const vec2 kCaptureSize = vec2( 16.f/9.f * 480.f, 480 ) ;
	// Very important lesson: if you aren't getting the native resolution (or something like it...)


const bool kDebug = false ;

const bool kAutoFullScreenProjector	= !kDebug ; // default: true
const bool kDrawCameraImage			= true  ; // default: false
const bool kDrawContours			= true ;
const bool kDrawContoursFilled		= kDebug ;  // default: false
const bool kDrawMouseDebugInfo		= kDebug ;
const bool kDrawPolyBoundingRect	= kDebug ;
const bool kDrawContourTree			= kDebug ;

//const bool kDebug = true ;
//
//const bool kAutoFullScreenProjector	= !kDebug ; // default: true
//const bool kDrawCameraImage			= true ; // default: false
//const bool kDrawContours			= false ;
//const bool kDrawContoursFilled		= false ;  // default: false
//const bool kDrawMouseDebugInfo		= true ;
//const bool kDrawPolyBoundingRect	= false ;
//const bool kDrawContourTree			= false ;

namespace cinder {
	
	PolyLine2 fromOcv( vector<cv::Point> pts )
	{
		PolyLine2 pl;
		for( auto p : pts ) pl.push_back(vec2(p.x,p.y));
		pl.setClosed(true);
		return pl;
	}

}

class cWindowData {
public:
	bool mIsAux = false ;
};


class Pipeline
{
public:
	void setQuery( string q ) { mQuery=q; } ;

	void then( Surface &img, string name )
	{
		if (name==mQuery)
		{
			mFrame = gl::Texture::create( img );
		}
	}
	
	void then( cv::Mat &img, string name )
	{
		if (name==mQuery)
		{
			mFrame = gl::Texture::create( fromOcv(img), gl::Texture::Format().loadTopDown() );
		}
	}
	
	// add types:
	// - contour
	
	gl::TextureRef getQueryFrame() const { return mFrame ; }
	
private:
	string		   mQuery ;
	gl::TextureRef mFrame ;
	
} ;

class PaperBounce3App : public App {
  public:
	void setup() override;
	void mouseDown( MouseEvent event ) override;
	void mouseMove( MouseEvent event ) override { mMousePos = event.getPos(); }
	void update() override;
	void draw() override;
	void resize() override;
	void keyDown( KeyEvent event ) override;
	
	void drawProjectorWindow() ;
	void drawAuxWindow() ;

	void updateVision(); // updates mCameraTexture, mContours
	
	Pipeline			mOCVPipelineTrace ;
	
	Font				mFont;
	gl::TextureFontRef	mTextureFont;
	
	
	CaptureRef			mCapture;
	
	ContourVector		mContours;
	
	app::WindowRef		mAuxWindow ; // for other debug info, on computer screen
	
	double				mLastFrameTime = 0. ;
	vec2				mMousePos ;
	
	// world info
	vec2 getWorldSize() const { return kCaptureSize ; }
		// units are um... pixels in camera space right now
		// but should switch to meters or something like that
	
	BallWorld mBallWorld ;
	
	// for main window, the projector display
	vec2 mouseToWorld( vec2 );
	void updateWindowMapping();
	
	float	mOrthoRect[4]; // points for glOrtho
};

void PaperBounce3App::setup()
{
	mLastFrameTime = getElapsedSeconds() ;
	
	// enumerate hardware
	auto displays = Display::getDisplays() ;
	cout << displays.size() << " Displays" << endl ;
	for ( auto d : displays )
	{
		cout << "\t" << d->getName() << " " << d->getBounds() << endl ;
	}

	auto cameras = Capture::getDevices() ;
	cout << cameras.size() << " Cameras" << endl ;
	for ( auto c : cameras )
	{
		cout << "\t" << c->getName() << endl ;
	}
	
	// get camera
	mCapture = Capture::create( kCaptureSize.x, kCaptureSize.y, cameras.back() ) ; // get last camera
	mCapture->start();

	// Fullscreen main window in secondary display
	if ( displays.size()>1 && kAutoFullScreenProjector )
	{
		getWindow()->setPos( displays[1]->getBounds().getUL() );
		
		getWindow()->setFullScreen(true) ;
	}

	// aux display
	if (0)
	{
		// for some reason this seems to create three windows once we fullscreen the main window :P
		app::WindowRef mAuxWindow = createWindow( Window::Format().size( kCaptureSize.x, kCaptureSize.y ) );
		
		cWindowData* data = new cWindowData ;
		data->mIsAux=true ;
		mAuxWindow->setUserData(data);
		
		if (displays.size()==1)
		{
			// move it out of the way on one screen
			mAuxWindow->setPos( getWindow()->getBounds().getUL() + ivec2(0,getWindow()->getBounds().getHeight() + 16.f) ) ;
		}
	}
	
	// text
	mTextureFont = gl::TextureFont::create( Font("Avenir",12) );
}

void PaperBounce3App::mouseDown( MouseEvent event )
{
	mBallWorld.newRandomBall( mouseToWorld( event.getPos() ) ) ;
}

void PaperBounce3App::update()
{
	updateVision();
	mBallWorld.update();
}

gl::Texture getImageSubarea( gl::Texture from, vec2 fromCoords[4], vec2 toSize )
{
	// from is the input camera image
	// fromCoords are the topleft, topright, b-r, b-l area we are going to cut out
	//	- specified in from's coordinate space
	// return value:
	//  - is the resulting image,
	//  - whose size is toSize
	
	// fbo (which we'll want to cache between frames :P)
	// etc
}

void PaperBounce3App::updateVision()
{
	if( mCapture->checkNewFrame() )
	{
		mOCVPipelineTrace = Pipeline() ;
		mOCVPipelineTrace.setQuery("input");
		
		// Get surface data
		Surface surface( *mCapture->getSurface() );
		
		// make cv frame
		cv::Mat input( toOcv( Channel( surface ) ) );
		cv::Mat output, gray, thresholded ;

		mOCVPipelineTrace.then( input, "input" );
		
//		cv::Sobel( input, output, CV_8U, 1, 0 );
		
		//		cv::threshold( input, output, 128, 255, CV_8U );
		//		cv::Laplacian( input, output, CV_8U );
		//		cv::circle( output, toOcv( Vec2f(200, 200) ), 300, toOcv( Color( 0, 0.5f, 1 ) ), -1 );
		//		cv::line( output, cv::Point( 1, 1 ), cv::Point( 30, 30 ), toOcv( Color( 1, 0.5f, 0 ) ) );

		
		// gray image
		if (1)
		{
//			cv::cvtColor( input, gray, cv::COLOR_BGR2GRAY ); // already grayscale
			
			cv::GaussianBlur( input, gray, cv::Size(5,5), 0 );
			mOCVPipelineTrace.then( gray, "gray" );

			cv::threshold( gray, thresholded, 0, 255, cv::THRESH_BINARY + cv::THRESH_OTSU );
			mOCVPipelineTrace.then( thresholded, "thresholded" );
		}
		
		// contours
		vector<vector<cv::Point> > contours;
		vector<cv::Vec4i> hierarchy;
		
		cv::findContours( thresholded, contours, hierarchy, cv::RETR_TREE, CV_CHAIN_APPROX_SIMPLE, cv::Point(0, 0) );
		
		// filter and process contours into our format
		mContours.clear() ;
		
		map<int,int> ocvContourIdxToMyContourIdx ;
		
		for( int i=0; i<contours.size(); ++i )
		{
			const auto &c = contours[i] ;
			
			cv::Point2f center ;
			float		radius ;
			
			cv::minEnclosingCircle( c, center, radius ) ;
			
			float		area = cv::contourArea(c) ;
			
			cv::RotatedRect rotatedRect = minAreaRect(c) ;
			
			if (	radius > kContourMinRadius &&
					area > kContourMinArea &&
					min( rotatedRect.size.width, rotatedRect.size.height ) > kContourMinWidth )
			{
				auto addContour = [&]( const vector<cv::Point>& c )
				{
					Contour myc ;
					
					myc.mPolyLine = fromOcv(c) ;
					myc.mRadius = radius ;
					myc.mCenter = fromOcv(center) ;
					myc.mArea   = area ;
//					myc.mIsHole = hierarchy[i][3] != -1 ;
					myc.mBoundingRect = Rectf( myc.mPolyLine.getPoints() );
					myc.mOcvContourIndex = i ;
					
					myc.mTreeDepth = 0 ;
					{
						int n = i ;
						while ( (n = hierarchy[n][3]) > 0 )
						{
							myc.mTreeDepth++ ;
						}
					}
					myc.mIsHole = ( myc.mTreeDepth % 2 ) ; // odd depth # children are holes
					myc.mIsLeaf = hierarchy[i][2] < 0 ;
					
					mContours.push_back( myc );
					
					// store my index mapping
					ocvContourIdxToMyContourIdx[i] = mContours.size()-1 ;
				} ;
				
				if (1)
				{
					// simplify
					vector<cv::Point> approx ;
					
					cv::approxPolyDP( c, approx, kContourDPEpislon, true ) ;
					
					addContour(approx);
				}
				else addContour(c) ;
			}
		}
		
		// add contour topology metadata
		// (this might be screwed up because of how we cull contours;
		// a simple fix might be to find orphaned contours--those with parents that don't exist anymore--
		// and strip them, too. but this will, in turn, force us to rebuild indices.
		// it might be simplest to just "hide" rejected contours, ignore them, but keep them around.
		for ( size_t i=0; i<mContours.size(); ++i )
		{
			Contour &c = mContours[i] ;
			
			if ( hierarchy[c.mOcvContourIndex][3] >= 0 )
			{
				c.mParent = ocvContourIdxToMyContourIdx[ hierarchy[c.mOcvContourIndex][3] ] ;
				
//				assert( myc.mParent is valid ) ;
				
				assert( c.mParent >= 0 && c.mParent < mContours.size() ) ;
				
				mContours[c.mParent].mChild.push_back( i ) ;
			}
		}
		
		// notify clients -> push changes
		mBallWorld.updateContours(mContours);
	}
}

void PaperBounce3App::resize()
{
	updateWindowMapping();
}

void PaperBounce3App::updateWindowMapping()
{
	auto ortho = [&]( float a, float b, float c, float d )
	{
		mOrthoRect[0] = a ;
		mOrthoRect[1] = b ;
		mOrthoRect[2] = c ;
		mOrthoRect[3] = d ;
	};
	
	// set window transform
	const float worldAspectRatio = getWorldSize().x / getWorldSize().y ;
	const float windowAspectRatio  = (float)getWindowSize().x / (float)getWindowSize().y ;
	
	if ( worldAspectRatio < windowAspectRatio )
	{
		// vertical black bars
		float w = windowAspectRatio * getWorldSize().y ;
		float barsw = w - getWorldSize().x ;
		
		ortho( -barsw/2, getWorldSize().x + barsw/2, getWorldSize().y, 0.f ) ;
	}
	else if ( worldAspectRatio > windowAspectRatio )
	{
		// horizontal black bars
		float h = (1.f / windowAspectRatio) * getWorldSize().x ;
		float barsh = h - getWorldSize().y ;
		
		ortho( 0.f, getWorldSize().x, getWorldSize().y + barsh/2, -barsh/2 ) ;
	}
	else ortho( 0.f, getWorldSize().x, getWorldSize().y, 0.f ) ;
}

vec2 PaperBounce3App::mouseToWorld( vec2 p )
{
	RectMapping rm( Rectf( 0.f, 0.f, getWindowSize().x, getWindowSize().y ),
					Rectf( mOrthoRect[0], mOrthoRect[3], mOrthoRect[1], mOrthoRect[2] ) ) ;
	
	return rm.map(p) ;
}

void PaperBounce3App::drawProjectorWindow()
{
	// set window transform
	gl::setMatrices( CameraOrtho( mOrthoRect[0], mOrthoRect[1], mOrthoRect[2], mOrthoRect[3], 0.f, 1.f ) ) ;
	
	// camera image
	if ( mOCVPipelineTrace.getQueryFrame() && kDrawCameraImage )
	{
		gl::color( 1, 1, 1 );
		gl::draw( mOCVPipelineTrace.getQueryFrame() );
	}
	
	// draw frame
	if (1)
	{
		gl::color( 1, 1, 1 );
		gl::drawStrokedRect( Rectf(0,0,getWorldSize().x,getWorldSize().y).inflated( vec2(-1,-1) ) ) ;
	}

	// draw contour bounding boxes, etc...
	if (kDrawPolyBoundingRect)
	{
		for( auto c : mContours )
		{
			if ( !c.mIsHole ) gl::color( 1.f, 1.f, 1.f, .2f ) ;
			else gl::color( 1.f, 0.f, .3f, .3f ) ;

			gl::drawStrokedRect(c.mBoundingRect);
		}
	}

	// draw contours
	if ( kDrawContours )
	{
		// filled
		if ( kDrawContoursFilled )
		{
			// recursive tree
			if (1)
			{
				function<void(const Contour&)> drawOne = [&]( const Contour& c )
				{
					if ( c.mIsHole ) gl::color(.0f,.0f,.0f,.8f);
					else gl::color(.2f,.2f,.4f,.5f);
					
					gl::drawSolid(c.mPolyLine);
					
					for( auto childIndex : c.mChild )
					{
						drawOne( mContours[childIndex] ) ;
					}
				};
				
				for( auto const &c : mContours )
				{
					if ( c.mTreeDepth==0 )
					{
						drawOne(c) ;
					}
				}
			}
			else
			// flat... (should work just the same, hmm)
			{
				// solids
				for( auto c : mContours )
				{
					if ( !c.mIsHole )
					{
						gl::color(.2f,.2f,.4f,.5f);
						gl::drawSolid(c.mPolyLine);
					}
				}
	
				// holes
				for( auto c : mContours )
				{
					if ( c.mIsHole )
					{
						gl::color(.0f,.0f,.0f,.8f);
						gl::drawSolid(c.mPolyLine);
					}
				}
			}
			
			// picked highlight
			vec2 mousePos = mouseToWorld(mMousePos) ;

			const Contour* picked = mContours.findLeafContourContainingPoint( mousePos ) ;
			
			if (picked)
			{
				if (picked->mIsHole) gl::color(6.f,.4f,.2f);
				else gl::color(.2f,.6f,.4f,.8f);
				
				gl::draw(picked->mPolyLine);
				
				if (0)
				{
					vec2 x = closestPointOnPoly(mousePos,picked->mPolyLine);
					gl::color(.5f,.1f,.3f,.9f);
					gl::drawSolidCircle(x, 5.f);
				}
			}
		}
		// outlines
		else
		{
			for( auto c : mContours )
			{
				ColorAf color ;
				
				if ( !c.mIsHole ) color = ColorAf(1,1,1); //color = ColorAf::hex( 0x43730F ) ;
				else color = ColorAf::hex( 0xF19878 ) ;
				
	//			color = ColorAf(1,1,1);
				
				gl::color(color) ;
				gl::draw(c.mPolyLine) ;
			}
		}
	}
	
	// draw balls
	mBallWorld.draw();
	
	// test collision logic
	if ( kDrawMouseDebugInfo && getWindowBounds().contains(mMousePos) )
	{
		vec2 pt = mouseToWorld( mMousePos ) ;
		
		float r = kBallDefaultRadius ;
		
//		vec2 surfaceNormal ;
		
		vec2 fixed = mBallWorld.resolveCollisionWithContours(pt,r);
		
		gl::color( ColorAf(0.f,0.f,1.f) ) ;
		gl::drawStrokedCircle(fixed,r);
		
		gl::color( ColorAf(0.f,1.f,0.f) ) ;
		gl::drawLine(pt, fixed);
		
//		gl::color( ColorAf(.8f,.2f,.3f,.5f) ) ;
//		gl::drawLine( fixed, fixed + surfaceNormal * r * 2.f ) ;
	}
	
	// draw contour debug info
	if (kDrawContourTree)
	{
		const float k = 16.f ;
		
		for ( size_t i=0; i<mContours.size(); ++i )
		{
			const auto& c = mContours[i] ;
			
			Rectf r = c.mBoundingRect ;
			
			r.y1 = ( c.mTreeDepth + 1 ) * k ;
			r.y2 = r.y1 + k ;
			
			if (c.mIsHole) gl::color(0,0,.3);
			else gl::color(0,.3,.4);
			
			gl::drawSolidRect(r) ;
			
			gl::color(.8,.8,.7);
			mTextureFont->drawString( to_string(i) + " (" + to_string(c.mParent) + ")",
				r.getLowerLeft() + vec2(2,-mTextureFont->getDescent()) ) ;
		}
	}
}

void PaperBounce3App::drawAuxWindow()
{
	if ( mOCVPipelineTrace.getQueryFrame() )
	{
		gl::setMatricesWindow( kCaptureSize.x, kCaptureSize.y );
		gl::color( 1, 1, 1 );
		gl::draw( mOCVPipelineTrace.getQueryFrame() );
	}
}

void PaperBounce3App::draw()
{
	gl::clear();
	gl::enableAlphaBlending();

	// I turned on MSAA=8, so these aren't needed.
	// They didn't work on polygons anyways.
	
//	gl::enable( GL_LINE_SMOOTH );
//	gl::enable( GL_POLYGON_SMOOTH );
	//	gl::enable( gl_point_smooth );
//	glHint( GL_LINE_SMOOTH_HINT, GL_NICEST );
//	glHint( GL_POLYGON_SMOOTH_HINT, GL_NICEST );
	//	glHint( GL_POINT_SMOOTH_HINT, GL_NICEST );
	

	cWindowData* winData = getWindow()->getUserData<cWindowData>() ;
	
	if ( winData && winData->mIsAux )
	{
		drawAuxWindow();
	}
	else
	{
		drawProjectorWindow();
	}
}

void PaperBounce3App::keyDown( KeyEvent event )
{
	switch ( event.getCode() )
	{
		case KeyEvent::KEY_f:
			cout << "Frame rate: " << getFrameRate() << endl ;
			break ;

		case KeyEvent::KEY_b:
			// make a random ball
			mBallWorld.newRandomBall( randVec2() * getWorldSize() ) ;
			// we could traverse paper hierarchy and pick a random point on paper...
			// might be a useful function, randomPointOnPaper()
			break ;
			
		case KeyEvent::KEY_c:
			mBallWorld.clearBalls() ;
			break ;
	}
}


CINDER_APP( PaperBounce3App, RendererGl(RendererGl::Options().msaa(8)), [&]( App::Settings *settings ) {
	settings->setFrameRate(120.f);
	settings->setWindowSize(kCaptureSize);
	settings->setTitle("See Paper") ;
})