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

#include "PipelineStageView.h"

#include "geom.h"
#include "xml.h"
#include "View.h"
#include "ocv.h"

#include <map>
#include <string>
#include <stdlib.h> // system()
#include <memory>

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
	void mouseUp( MouseEvent event ) override;
	void mouseMove( MouseEvent event ) override;
	void mouseDrag( MouseEvent event ) override;
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
	
	vec2				getMousePosInWindow() const { return mMousePosInWindow; }
	vec2				mMousePosInWindow;
	
	// to help us visualize
	void addProjectorPipelineStages();
	void updatePipelineViews( bool areViewsVisible );
	
	ViewCollection mViews;
	
	std::shared_ptr<MainImageView> mMainImageView; // main view, with image in it.
	void updateMainImageTransform(); // configure mMainImageView's transform
	
	/* Coordinates spaces, there are a few:
		
		UI (window coordinates, in points, not pixels)
			- pixels
			- points
			
		Image
			- Camera
				e.g. pixel location in capture image
			- Projector
				e.g. pixel location on a screen
			- Arbitrary
				e.g. supersampled camera image subset
				e.g. location of a pixel on a shown image
		
		World
			(sim size, unbounded)	eg. location of a bouncing ball
	 
	 
	Transforms
		UI    <> Image -- handled by View objects
		Image <> World -- currently handled by Pipeline::Stage transforms

	*/
	

	
	// settings
	bool mAutoFullScreenProjector = false ;
	bool mDrawCameraImage = false ;
	bool mDrawContours = false ;
	bool mDrawContoursFilled = false ;
	bool mDrawMouseDebugInfo = false ;
	bool mDrawPolyBoundingRect = false ;
	bool mDrawContourTree = false ;
	bool mDrawPipeline = false;
	bool mDrawContourMousePick = false;

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
			getXml(app,"DrawPipeline",mDrawPipeline);
			getXml(app,"DrawContourMousePick",mDrawContourMousePick);
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
	if ( mPipeline.getQuery().empty() )
	{
		if (mAutoFullScreenProjector) mPipeline.setQuery("projector");
		else mPipeline.setQuery("clipped");
	}
	mPipeline.setCaptureAllStageImages(mDrawPipeline);
	
	// make main image view
	mMainImageView = make_shared<MainImageView>( MainImageView( mPipeline, mBallWorld ) );
	mMainImageView->mWorldDrawFunc = [&](){drawWorld();};
		// draw all the contours, etc... as well as the game world itself.
	mViews.addView(mMainImageView);
	
	// poly editors
	// - camera capture coords
	{
		std::shared_ptr<PolyEditView> cameraPolyEditView = make_shared<PolyEditView>(
			PolyEditView(
				mPipeline,
				PolyLine2(vector<vec2>(mLightLink.mCaptureCoords,mLightLink.mCaptureCoords+4)),
				"input"
				)
			);
		
		cameraPolyEditView->setPolyFunc( [&]( const PolyLine2& poly ){
			assert( poly.getPoints().size()==4 );
			for( int i=0; i<4; ++i ) mLightLink.mCaptureCoords[i] = poly.getPoints()[i];
			mVision.setLightLink(mLightLink);
		});
		
		cameraPolyEditView->setMainImageView( mMainImageView );
		cameraPolyEditView->getEditableInStages().push_back("input");
		
		mViews.addView( cameraPolyEditView );
	}

	// - projector mapping
	if (1)
	{
		PolyLine2 poly(vector<vec2>(mLightLink.mProjectorCoords,mLightLink.mProjectorCoords+4));
		
		// convert to input coords
		std::shared_ptr<PolyEditView> projPolyEditView = make_shared<PolyEditView>(
			PolyEditView(
				mPipeline,
				PolyLine2(vector<vec2>(mLightLink.mProjectorCoords,mLightLink.mProjectorCoords+4)),
				"projector"
				)
		);
		
		projPolyEditView->setPolyFunc( [&]( const PolyLine2& poly ){
			assert( poly.getPoints().size()==4 );
			for( int i=0; i<4; ++i ) mLightLink.mProjectorCoords[i] = poly.getPoints()[i];
			mVision.setLightLink(mLightLink);
		});
		
		projPolyEditView->setMainImageView( mMainImageView );
		projPolyEditView->getEditableInStages().push_back("projector");
		
		mViews.addView( projPolyEditView );
	}

	// TODO:
	// - colors per poly
	// - √ set data after editing (lambda?)
	// - √ specify native coordinate system
}

fs::path
PaperBounce3App::myGetAssetPath( fs::path p ) const
{
	if ( mOverloadedAssetPath.empty() ) return getAssetPath(p);
	else return fs::path(mOverloadedAssetPath) / p ;
}

void PaperBounce3App::mouseDown( MouseEvent event )
{
	mMousePosInWindow = event.getPos();
	mViews.mouseDown(event);
}

void PaperBounce3App::mouseUp( MouseEvent event )
{
	mMousePosInWindow = event.getPos();
	mViews.mouseUp(event);
}

void PaperBounce3App::mouseMove( MouseEvent event )
{
	mMousePosInWindow = event.getPos();
	mViews.mouseMove(event);
}

void PaperBounce3App::mouseDrag( MouseEvent event )
{
	mMousePosInWindow = event.getPos();
	mViews.mouseDrag(event);
}

void PaperBounce3App::addProjectorPipelineStages()
{
	// set that image
	mPipeline.then( mLightLink.mProjectorSize, "projector" ) ;
	
	mPipeline.setImageToWorldTransform(
		getOcvPerspectiveTransform(
			mLightLink.mProjectorCoords,
			mLightLink.mProjectorWorldSpaceCoords ));
}


void PaperBounce3App::updatePipelineViews( bool areViewsVisible )
{
	const float kUIGutter = 8.f ;
	const float kUIWidth  = 64.f ;
	
	const auto stages = mPipeline.getStages();
	
	vec2 pos(kUIGutter,kUIGutter);
	
	for( const auto &s : stages )
	{
		// view exists?
		ViewRef view = mViews.getViewByName(s.mName);
		
		// erase it?
		if ( view && !areViewsVisible )
		{
			mViews.removeView(view);
			view=0;
		}
		
		// make a new one?
		if ( !view && areViewsVisible )
		{
			PipelineStageView psv( mPipeline, s.mName );
			psv.setWorldDrawFunc( [&](){ drawWorld(); } );
			
			view = make_shared<PipelineStageView>(psv);
			
			view->setName(s.mName);
			
			mViews.addView(view);
		}
		
		// configure it
		if (view)
		{
			// update its location
			vec2 size = s.mImageSize;
			
			size *= kUIWidth / size.x ;
			
			view->setFrame ( Rectf(pos, pos + size) );
			view->setBounds( Rectf(vec2(0,0), s.mImageSize) );
			
			// next pos
			pos = view->getFrame().getLowerLeft() + vec2(0,kUIGutter);
		}
	}
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
		
		// update pipeline visualization
		updatePipelineViews( mDrawPipeline );
		
		// since the pipeline stage we are drawing might have changed... (or come into existence)
		// update the window mapping
		// this is a little weird to put here, but ah well
		updateMainImageTransform();
	}
	
	mBallWorld.update();
}

void PaperBounce3App::resize()
{
	updateMainImageTransform();
}

void PaperBounce3App::updateMainImageTransform()
{
	// get content bounds
	Rectf bounds;

	{
		// in terms of size...
		// this is a little weird, but it's basically historical code that needs to be
		// refactored.
		vec2 drawSize;
		
		if ( mPipeline.getQueryStage() )
		{
			// should always be the case
			drawSize = mPipeline.getQueryStage()->mImageSize;
		}
		else
		{
			// world by default.
			// (deprecated; not really sure what this even means anymore.)
			drawSize = getWorldSize();
		}
		
		bounds = Rectf( 0, 0, drawSize.x, drawSize.y );
	}
	
	mMainImageView->setBounds( bounds );

	
	// it fills the window
	const Rectf windowRect = Rectf(0,0,getWindowSize().x,getWindowSize().y);
	const Rectf frame      = bounds.getCenteredFit( windowRect, true );
	
	mMainImageView->setFrame( frame );
}

void PaperBounce3App::drawProjectorWindow()
{
	// ====== Window Space (UI) =====
	// baseline coordinate space
	gl::setMatricesWindow( getWindowSize() );
	
	// ====== Window Space (UI) =====
	drawUI();
}

void PaperBounce3App::drawWorld()
{
	const vec2 mouseInWindow   = getMousePosInWindow();
	const vec2 mouseInWorld    = mMainImageView->mouseToWorld(mouseInWindow);
	const bool isMouseInWindow = getWindowBounds().contains(getMousePosInWindow());
	
	
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

		if ( mDrawContourMousePick )
		{
			// picked highlight
			const Contour* picked = mContours.findLeafContourContainingPoint( mouseInWorld ) ;
			
			if (picked)
			{
				if (picked->mIsHole) gl::color(6.f,.4f,.2f);
				else gl::color(.2f,.6f,.4f,.8f);
				
				gl::draw(picked->mPolyLine);
				
				if (0)
				{
					vec2 x = closestPointOnPoly(mouseInWorld,picked->mPolyLine);
					gl::color(.5f,.1f,.3f,.9f);
					gl::drawSolidCircle(x, 5.f);
				}
			}
		}
	}
	
	// draw balls
	mBallWorld.draw();
	
	// mouse debug info
	if ( mDrawMouseDebugInfo && isMouseInWindow )
	{
		// test collision logic
		const float r = mBallWorld.getBallDefaultRadius() ;
		
		vec2 fixed = mBallWorld.resolveCollisionWithContours(mouseInWorld,r);
		
		gl::color( ColorAf(0.f,0.f,1.f) ) ;
		gl::drawStrokedCircle(fixed,r);
		
		gl::color( ColorAf(0.f,1.f,0.f) ) ;
		gl::drawLine(mouseInWorld, fixed);
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
	mViews.draw();
	
	// this, below, could become its own view
	// it would need a shared_ptr to MainImageView (no biggie)
	if ( mDrawMouseDebugInfo )//&& getWindowBounds().contains(getMousePosInWindow()) )
	{
		const vec2 pt = getMousePosInWindow();
		
		// coordinates
		vec2   o[2] = { vec2(.5,1.5), vec2(0,0) };
		ColorA c[2] = { ColorA(0,0,0), ColorA(1,1,1) };
		
		for( int i=0; i<2; ++i )
		{
			gl::color( c[i] );
			mTextureFont->drawString(
				"Window: " + toString(pt) +
				"\tImage: "  + toString( mMainImageView->mouseToImage(pt) ) +
				"\tWorld: " + toString( mMainImageView->mouseToWorld(pt) )
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
			//
			// how to generalize this?
			// pass unhandled key events into GameWorld?
			break ;
			
		case KeyEvent::KEY_c:
			mBallWorld.clearBalls() ;
			break ;

		case KeyEvent::KEY_x:
			::system( (string("open \"") + myGetAssetPath("config.xml").string() + "\"").c_str() );
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