#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/Rand.h"

#include "cinder/Text.h"
#include "cinder/gl/TextureFont.h"
#include "cinder/Xml.h"

#include "cinder/Capture.h"
#include "CinderOpenCV.h"

#include "LightLink.h"
#include "Vision.h"
#include "Contour.h"
#include "BallWorld.h"
#include "XmlFileWatch.h"
#include "Pipeline.h"

#include "geom.h"
#include "xml.h"
#include "View.h"
#include "ocv.h"

#include <map>
#include <string>

using namespace ci;
using namespace ci::app;
using namespace std;


const int  kRequestFrameRate = 60 ; // was 120, don't think it matters/we got there
const vec2 kDefaultWindowSize( 640, 480 );



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
	
	void drawWorld();
	void drawUI();
	
	LightLink			mLightLink; // calibration for camera <> world <> projector
	CaptureRef			mCapture;	// input device		->
	Vision				mVision ;	// edge detection	->
	ContourVector		mContours;	// edges output		->
	BallWorld			mBallWorld ;// world simulation
	
	Pipeline			mPipeline; // traces processing
	
	// world info
	vec2 getWorldSize() const ;
		// world rect might be more appropriate...
		// this is kind of a deprecated concept; it is used to:
		// - draw a frame
		// - spawn random balls
		// What are we talking about? Replace with these concepts:
		// - contour boundaries?
		// - camera world bounds?
		// - projector world bounds?
		// - union of camera + projector bounds?
		// - some other arbitrary thing?
	
	
	// ui
	Font				mFont;
	gl::TextureFontRef	mTextureFont;
	
	app::WindowRef		mAuxWindow ; // for other debug info, on computer screen
	
	double				mLastFrameTime = 0. ;
	vec2				mMousePos ;
	
	
	// for main window,
	vec2 mouseToImage( vec2 ); // maps mouse (screen) to drawn image coords
	vec2 mouseToWorld( vec2 ); // maps mouse (screen) to world coordinates
	void updateWindowMapping(); // maps world coordinates to the projector display (and back)
	
	float	mOrthoRect[4]; // points for glOrtho; set by updateWindowMapping()
	
	/* Coordinates spaces, there are a few:
	
	- Window coordinate space		(window size),			eg. mouse coordinate
	
		^^ ortho transform (Texture <> Window : mOrthoRect)
		||
		
	- Texture coordinate space			(texture size),			eg. location of a pixel on a shown image
		- Camera coordinate space		(capture size),			eg. pixel location in capture image
		- Projector coordinate space	(screen size),			eg. pixel location on a screen
		
		^^ world to image transform
		||
		
	- World coordinate space,		(sim size, unbounded)	eg. location of a bouncing ball

	*/
	
	// to help us visualize
	void addProjectorPipelineStages();
	
	// settings
	bool mAutoFullScreenProjector = false ;
	bool mDrawCameraImage = false ;
	bool mDrawContours = false ;
	bool mDrawContoursFilled = false ;
	bool mDrawMouseDebugInfo = false ;
	bool mDrawPolyBoundingRect = false ;
	bool mDrawContourTree = false ;

	fs::path myGetAssetPath( fs::path ) const ; // prepends the appropriate thing...
	string mOverloadedAssetPath;
	
	XmlFileWatch mXmlFileWatch;
};

vec2 PaperBounce3App::getWorldSize() const
{
	vec2 lo=mLightLink.mCaptureWorldSpaceCoords[0], hi=mLightLink.mCaptureWorldSpaceCoords[0];
	
	for( int i=1; i<4; ++i )
	{
		lo = min( lo, mLightLink.mCaptureWorldSpaceCoords[i] );
		hi = max( hi, mLightLink.mCaptureWorldSpaceCoords[i] );
	}
	
	return hi - lo ;
}

void PaperBounce3App::setup()
{
	// command line args
	auto args = getCommandLineArgs();
	
	for( size_t a=0; a<args.size(); ++a )
	{
		if ( args[a]=="-assets" && args.size()>a )
		{
			mOverloadedAssetPath = args[a+1];
		}
	}
	
	//
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
	
	// configuration
	mXmlFileWatch.load( myGetAssetPath("config.xml"), [this]( XmlTree xml )
	{
		// 1. get params
		if (xml.hasChild("PaperBounce3/BallWorld"))
		{
			mBallWorld.setParams(xml.getChild("PaperBounce3/BallWorld"));
		}
		
		if (xml.hasChild("PaperBounce3/Vision"))
		{
			mVision.setParams(xml.getChild("PaperBounce3/Vision"));
		}

		if (xml.hasChild("PaperBounce3/LightLink"))
		{
			mLightLink.setParams(xml.getChild("PaperBounce3/LightLink"));
		}
		
		if (xml.hasChild("PaperBounce3/App"))
		{
			XmlTree app = xml.getChild("PaperBounce3/App");
			
			getXml(app,"AutoFullScreenProjector",mAutoFullScreenProjector);
			getXml(app,"DrawCameraImage",mDrawCameraImage);
			getXml(app,"DrawContours",mDrawContours);
			getXml(app,"DrawContoursFilled",mDrawContoursFilled);
			getXml(app,"DrawMouseDebugInfo",mDrawMouseDebugInfo);
			getXml(app,"DrawPolyBoundingRect",mDrawPolyBoundingRect);
			getXml(app,"DrawContourTree",mDrawContourTree);
		}

		// 2. respond
		
		mVision.setLightLink(mLightLink);
		
		// TODO:
		// - get a new camera capture object so that resolution can change live
		// - respond to fullscreen projector flag
		// - aux display config
	});

	// resize window
	setWindowSize( mLightLink.getCaptureSize().x, mLightLink.getCaptureSize().y ) ;
	
	// get camera
	mCapture = Capture::create( mLightLink.getCaptureSize().x, mLightLink.getCaptureSize().y, cameras.back() ) ; // get last camera
	mCapture->start();

	// Fullscreen main window in secondary display
	if ( displays.size()>1 && mAutoFullScreenProjector )
	{
		getWindow()->setPos( displays[1]->getBounds().getUL() );
		
		getWindow()->setFullScreen(true) ;
	}

	// aux display
	if (0)
	{
		// for some reason this seems to create three windows once we fullscreen the main window :P
		app::WindowRef mAuxWindow = createWindow( Window::Format().size( mLightLink.getCaptureSize().x, mLightLink.getCaptureSize().y ) );
		
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

	// default pipeline image to query
	// (after loading xml)
	// (we query before making the pipeline because we only store the image requested :P!)
	if (mAutoFullScreenProjector) mPipeline.setQuery("projector");
	else mPipeline.setQuery("clipped");
}

fs::path
PaperBounce3App::myGetAssetPath( fs::path p ) const
{
	if ( mOverloadedAssetPath.empty() ) return getAssetPath(p);
	else return fs::path(mOverloadedAssetPath) / p ;
}

void PaperBounce3App::mouseDown( MouseEvent event )
{
	mBallWorld.newRandomBall( mouseToWorld( event.getPos() ) ) ;
}

void PaperBounce3App::addProjectorPipelineStages()
{
	// set that image
	mPipeline.then( mLightLink.mProjectorSize, "projector" ) ;
	
	// get the transform
	/*
	cv::Point2f srcpt[4], dstpt[4];
	
	for( int i=0; i<4; ++i )
	{
		srcpt[i] = toOcv( mLightLink.mProjectorCoords[i] );
		dstpt[i] = toOcv( mLightLink.mProjectorWorldSpaceCoords[i] );
	}

	cv::Mat xform = cv::getPerspectiveTransform( srcpt, dstpt ) ;
	
	mPipeline.setImageToWorldTransform( mat3to4( fromOcvMat3x3(xform) ) );*/
	
	mPipeline.setImageToWorldTransform(
		getOcvPerspectiveTransform(
			mLightLink.mProjectorCoords,
			mLightLink.mProjectorWorldSpaceCoords ));
}

void PaperBounce3App::update()
{
	mXmlFileWatch.update();
	
	if ( mCapture->checkNewFrame() )
	{
		// start pipeline
		mPipeline.start();

		// get image
		Surface frame( *mCapture->getSurface() ) ;
		
		// vision it
		mVision.processFrame(frame,mPipeline) ;
		
		// finish off the pipeline with draw stage
		addProjectorPipelineStages();
		
		// pass contours to ballworld (probably don't need to store here)
		mContours = mVision.mContourOutput ;
		
		mBallWorld.updateContours( mContours );
		
		// since the pipeline stage we are drawing might have changed... (or come into existence)
		// update the window mapping
		// this is a little weird to put here, but ah well
		updateWindowMapping();
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
	vec2 drawSize = getWorldSize(); // world by default
	
	if ( mPipeline.getQueryStage() )
	{
		drawSize = mPipeline.getQueryStage()->mImageSize;
	}
	
	const float drawAspectRatio    = drawSize.x / drawSize.y ;
	const float windowAspectRatio  = (float)getWindowSize().x / (float)getWindowSize().y ;
	
	if ( drawAspectRatio < windowAspectRatio )
	{
		// vertical black bars
		float w = windowAspectRatio * drawSize.y ;
		float barsw = w - drawSize.x ;
		
		ortho( -barsw/2, drawSize.x + barsw/2, drawSize.y, 0.f ) ;
	}
	else if ( drawAspectRatio > windowAspectRatio )
	{
		// horizontal black bars
		float h = (1.f / windowAspectRatio) * drawSize.x ;
		float barsh = h - drawSize.y ;
		
		ortho( 0.f, drawSize.x, drawSize.y + barsh/2, -barsh/2 ) ;
	}
	else ortho( 0.f, drawSize.x, drawSize.y, 0.f ) ;
}

vec2 PaperBounce3App::mouseToImage( vec2 p )
{
	// convert screen/window coordinates to drawn texture (image) coords
	RectMapping rm( Rectf( 0.f, 0.f, getWindowSize().x, getWindowSize().y ),
					Rectf( mOrthoRect[0], mOrthoRect[3], mOrthoRect[1], mOrthoRect[2] ) ) ;
	
	return rm.map(p) ;
}

vec2 PaperBounce3App::mouseToWorld( vec2 p )
{
	// convert image coordinates to world coords
	vec2 p2 = mouseToImage(p);
	
	if (mPipeline.getQueryStage())
	{
		p2 = vec2( mPipeline.getQueryStage()->mImageToWorld * vec4(p2,0,1) ) ;
	}
	
	return p2;
}

void PaperBounce3App::drawProjectorWindow()
{
	// ===== Texture Space of Visualized Pipeline Stage =====
	
	// set window transform
	//	coord space is in texture space of the pipeline stage we are drawing
	gl::setMatrices( CameraOrtho( mOrthoRect[0], mOrthoRect[1], mOrthoRect[2], mOrthoRect[3], 0.f, 1.f ) ) ;
	
	// vision pipeline image
	//
	if ( mPipeline.getQueryStage() &&
		 mPipeline.getQueryStage()->mImage &&
		 mDrawCameraImage )
	{
		gl::color( 1, 1, 1 );
		
		gl::draw( mPipeline.getQueryStage()->mImage );
	}
	
	// ===== World space =====
	gl::pushViewMatrix();
	
	if (mPipeline.getQueryStage())
	{
		// convert world coordinates to drawn texture coords
		gl::multViewMatrix( mPipeline.getQueryStage()->mWorldToImage );
	}
	
	drawWorld();
	
	
	// ====== Window Space (UI)
	
	gl::popViewMatrix();
	gl::setMatrices( CameraOrtho( 0, getWindowSize().x, getWindowSize().y, 0, 0.f, 1.f ) ) ;

	drawUI();
	
	//
	if (0)
	{
		View v;
		
		v.mFrame  = Rectf(0,0,320,240) + vec2(100,100);
		v.mBounds = Rectf(0,0,640,480);
		
		gl::pushViewMatrix();
		gl::multViewMatrix( v.getChildToParentMatrix() );
		
	//	cout << v.getChildToParentMatrix() << endl;
	//	cout << glm::translate( vec3(100, 100, 0.f) ) << endl;
		
		v.draw();
		
		gl::popViewMatrix();
	}
}

void PaperBounce3App::drawWorld()
{
	// draw frame
	if (0)
	{
		gl::color( 1, 1, 1 );
		gl::drawStrokedRect( Rectf(0,0,getWorldSize().x,getWorldSize().y).inflated( vec2(-1,-1) ) ) ;
	}

	// draw contour bounding boxes, etc...
	if (mDrawPolyBoundingRect)
	{
		for( auto c : mContours )
		{
			if ( !c.mIsHole ) gl::color( 1.f, 1.f, 1.f, .2f ) ;
			else gl::color( 1.f, 0.f, .3f, .3f ) ;

			gl::drawStrokedRect(c.mBoundingRect);
		}
	}

	// draw contours
	if ( mDrawContours )
	{
		// filled
		if ( mDrawContoursFilled )
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
				
				if ( !c.mIsHole ) color = ColorAf(1,1,1);
				else color = ColorAf::hex( 0xF19878 ) ;
				
				gl::color(color) ;
				gl::draw(c.mPolyLine) ;
			}
		}
	}
	
	// draw balls
	mBallWorld.draw();
	
	// mouse debug info
	if ( mDrawMouseDebugInfo && getWindowBounds().contains(mMousePos) )
	{
		// test collision logic
		vec2 pt = mouseToWorld( mMousePos ) ;
		
		const float r = mBallWorld.getBallDefaultRadius() ;
		
		vec2 fixed = mBallWorld.resolveCollisionWithContours(pt,r);
		
		gl::color( ColorAf(0.f,0.f,1.f) ) ;
		gl::drawStrokedCircle(fixed,r);
		
		gl::color( ColorAf(0.f,1.f,0.f) ) ;
		gl::drawLine(pt, fixed);
	}
	
	// draw contour debug info
	if (mDrawContourTree)
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

void PaperBounce3App::drawUI()
{
	if ( mDrawMouseDebugInfo && getWindowBounds().contains(mMousePos) )
	{
		vec2 pt = mMousePos;
		
		// coordinates
		vec2   o[2] = { vec2(.5,1.5), vec2(0,0) };
		ColorA c[2] = { ColorA(0,0,0), ColorA(1,1,1) };
		
		for( int i=0; i<2; ++i )
		{
			gl::color( c[i] );
			mTextureFont->drawString(
				"Window: " + toString(pt) +
				"\tImage: "  + toString( mouseToImage(pt) ) +
				"\tWorld: " + toString( mouseToWorld(pt) )
				, o[i]+vec2( 8.f, getWindowSize().y - mTextureFont->getAscent()+mTextureFont->getDescent()) ) ;
		}
	}
}

void PaperBounce3App::drawAuxWindow()
{
//	if ( mPipeline.getQueryFrame() )
//	{
//		gl::setMatricesWindow( mLightLink.getCaptureSize().x, mLightLink.getCaptureSize().y );
//		gl::color( 1, 1, 1 );
//		gl::draw( mPipeline.getQueryFrame() );
//	}
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
			
		case KeyEvent::KEY_UP:
			if (event.isMetaDown()) mPipeline.setQuery( mPipeline.getFirstStageName() );
			else mPipeline.setQuery( mPipeline.getPrevStageName(mPipeline.getQuery() ) );
			break;
			
		case KeyEvent::KEY_DOWN:
			if (event.isMetaDown()) mPipeline.setQuery( mPipeline.getLastStageName() );
			else mPipeline.setQuery( mPipeline.getNextStageName(mPipeline.getQuery() ) );
			break;

	}
}


CINDER_APP( PaperBounce3App, RendererGl(RendererGl::Options().msaa(8)), [&]( App::Settings *settings ) {
	settings->setFrameRate(kRequestFrameRate);
	settings->setWindowSize(kDefaultWindowSize);
	settings->setTitle("See Paper") ;
})