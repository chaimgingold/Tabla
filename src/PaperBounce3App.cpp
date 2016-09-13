#include "PaperBounce3App.h"

#include "geom.h"
#include "xml.h"
#include "View.h"
#include "ocv.h"

#include <map>
#include <string>
#include <memory>

#include <stdlib.h> // system()

const int  kRequestFrameRate = 60 ; // was 120, don't think it matters/we got there
const vec2 kDefaultWindowSize( 640, 480 );


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

	// get camera
	mCapture = Capture::create( mLightLink.getCaptureSize().x, mLightLink.getCaptureSize().y, cameras.back() ) ; // get last camera
	mCapture->start();

	// setup main window
	mMainWindow = getWindow();
	mMainWindow->setTitle("Projector");
	mMainWindow->setUserData( new WindowData(mMainWindow,false,*this) );

	// resize window
	setWindowSize( mLightLink.getCaptureSize().x, mLightLink.getCaptureSize().y ) ;

	// Fullscreen main window in secondary display
	if ( displays.size()>1 && mAutoFullScreenProjector )
	{
		// move to 2nd display
		mMainWindow->setPos( displays[1]->getBounds().getUL() );
		
		mMainWindow->setFullScreen(true) ;
	}

	// aux UI display
	if (1)
	{
		// for some reason this seems to create three windows once we fullscreen the main window :P
		mUIWindow = createWindow( Window::Format().size( mLightLink.getCaptureSize().x, mLightLink.getCaptureSize().y ) );
		
		mUIWindow->setTitle("Configuration");
		mUIWindow->setUserData( new WindowData(mUIWindow,true,*this) );
		
		// move it out of the way on one screen
		if ( mMainWindow->getDisplay() == mUIWindow->getDisplay() )
		{
			mUIWindow->setPos( mMainWindow->getPos() + ivec2( -mMainWindow->getWidth()-16,0) ) ;
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
}

fs::path
PaperBounce3App::myGetAssetPath( fs::path p ) const
{
	if ( mOverloadedAssetPath.empty() ) return getAssetPath(p);
	else return fs::path(mOverloadedAssetPath) / p ;
}

void PaperBounce3App::mouseDown( MouseEvent event )
{
	getWindow()->getUserData<WindowData>()->mouseDown(event);
}

void PaperBounce3App::mouseUp( MouseEvent event )
{
	getWindow()->getUserData<WindowData>()->mouseUp(event);
}

void PaperBounce3App::mouseMove( MouseEvent event )
{
	getWindow()->getUserData<WindowData>()->mouseMove(event);
}

void PaperBounce3App::mouseDrag( MouseEvent event )
{
	getWindow()->getUserData<WindowData>()->mouseDrag(event);
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
	// only do UI window
	if (!mUIWindow) return;
	WindowData *win = mUIWindow->getUserData<WindowData>();
	if (!win) return;
	ViewCollection& views = win->getViews();
	
	const float kUIGutter = 8.f ;
	const float kUIWidth  = 64.f ;
	
	const auto stages = mPipeline.getStages();
	
	vec2 pos(kUIGutter,kUIGutter);
	
	for( const auto &s : stages )
	{
		// view exists?
		ViewRef view = views.getViewByName(s.mName);
		
		// erase it?
		if ( view && !areViewsVisible )
		{
			views.removeView(view);
			view=0;
		}
		
		// make a new one?
		if ( !view && areViewsVisible )
		{
			PipelineStageView psv( mPipeline, s.mName );
			psv.setWorldDrawFunc( [&](){ drawWorld(); } );
			
			view = make_shared<PipelineStageView>(psv);
			
			view->setName(s.mName);
			
			views.addView(view);
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
		updateMainImageTransform(mUIWindow);
		updateMainImageTransform(mMainWindow);
	}
	
	mBallWorld.update();
}

void PaperBounce3App::updateMainImageTransform( WindowRef w )
{
	if (w)
	{
		WindowData *win = w->getUserData<WindowData>();
		
		// might not have been created yet (e.g., in setup)
		if (win) win->updateMainImageTransform();
	}
}

void PaperBounce3App::resize()
{
	updateMainImageTransform(getWindow());
}

void PaperBounce3App::drawWorld()
{
	WindowData *win = getWindow()->getUserData<WindowData>();
	
	const vec2 mouseInWindow   = win->getMousePosInWindow();
	const vec2 mouseInWorld    = win->getMainImageView()->mouseToWorld(mouseInWindow);
	const bool isMouseInWindow = getWindowBounds().contains(win->getMousePosInWindow());
	
	
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

void PaperBounce3App::draw()
{
	WindowData* win = getWindow()->getUserData<WindowData>() ;
	
	if (win) win->draw();
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