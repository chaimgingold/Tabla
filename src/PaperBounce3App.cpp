#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/Rand.h"

#include "cinder/Text.h"
#include "cinder/gl/TextureFont.h"

#include "cinder/Capture.h"
#include "CinderOpenCV.h"

#include "Vision.h"
#include "Contour.h"
#include "BallWorld.h"

#include "geom.h"

#include <map>
#include <string>

using namespace ci;
using namespace ci::app;
using namespace std;



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


class cWindowData {
public:
	bool mIsAux = false ;
};


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
	
	CaptureRef			mCapture;	// input device		->
	Vision				mVision ;	// edge detection	->
	ContourVector		mContours;	// edges output		->
	BallWorld			mBallWorld ;// world simulation
	
	// world info
	vec2 getWorldSize() const { return kCaptureSize ; }
		// units are um... pixels in camera space right now
		// but should switch to meters or something like that
	
	
	// ui
	Font				mFont;
	gl::TextureFontRef	mTextureFont;
	
	app::WindowRef		mAuxWindow ; // for other debug info, on computer screen
	
	double				mLastFrameTime = 0. ;
	vec2				mMousePos ;
	
	
	// for main window,
	vec2 mouseToWorld( vec2 ); // maps mouse to world coordinates
	void updateWindowMapping(); // maps world coordinates to the projector display (and back)
	
	float	mOrthoRect[4]; // points for glOrtho; set by updateWindowMapping()
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
	if ( mCapture->checkNewFrame() )
	{
		Surface frame( *mCapture->getSurface() ) ;
		
		mVision.processFrame(frame) ;
		
		mContours = mVision.mContourOutput ;
		
		mBallWorld.updateContours( mContours );
	}
	
	mBallWorld.update();
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
	
	// vision pipeline image
	if ( mVision.mOCVPipelineTrace.getQueryFrame() && kDrawCameraImage )
	{
		gl::color( 1, 1, 1 );
		gl::draw( mVision.mOCVPipelineTrace.getQueryFrame() );
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
	if ( mVision.mOCVPipelineTrace.getQueryFrame() )
	{
		gl::setMatricesWindow( kCaptureSize.x, kCaptureSize.y );
		gl::color( 1, 1, 1 );
		gl::draw( mVision.mOCVPipelineTrace.getQueryFrame() );
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