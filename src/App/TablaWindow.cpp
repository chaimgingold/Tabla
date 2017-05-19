//
//  TablaWindow.cpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 9/12/16.
//
//

#include "TablaWindow.h"
#include "TablaApp.h"
#include "geom.h" // getPointsAsPoly

#include "GameLibraryView.h"
#include "PolyEditView.h"
#include "VectorEditView.h"
#include "MainImageView.h"
#include "CaptureProfileMenuView.h"

const float kZNear =  100.f;
const float kZFar  = -100.f;

TablaWindow::TablaWindow( WindowRef window, bool isUIWindow, TablaApp& app )
	: mApp(app)
	, mWindow(window)
	, mIsUIWindow(isUIWindow)
{
	mWindow->setUserData(this);
	mWindow->setTitle( isUIWindow ? "Configuration" : "Projector" );
	
	mMainImageView = make_shared<MainImageView>( MainImageView(mApp) );
	mMainImageView->setIsProjectorView(!isUIWindow);
	mMainImageView->mWorldDrawFunc = [this](){
		mApp.drawWorld( getIsUIWindow() ? GameWorld::DrawType::UIMain : GameWorld::DrawType::Projector );
	};
		// draw all the contours, etc... as well as the game world itself.
	mViews.addView(mMainImageView);
	
	// customize it
	if ( mIsUIWindow )
	{
		// margin
		Rectf margin = Rectf(1,1,1,1) * mApp.getParams().mConfigWindowMainImageMargin;
		margin.x1 = max( margin.x1, mApp.getParams().mConfigWindowPipelineWidth + mApp.getParams().mConfigWindowPipelineGutter*2.f );
		mMainImageView->setMargin( margin );
		mMainImageView->setFrameColor( ColorA(1,1,1,.35f) );
		mMainImageView->setFont(mApp.getFont());
	}
	else
	{
		// lock it into this stage
		mMainImageView->setPipelineStageName("projector");
	}

	// add ui elements
	if ( mIsUIWindow )
	{
		addPolyEditors();
		addMenus();
	}
}

void TablaWindow::addPolyEditors()
{
	// TODO:
	// - colors per poly
	// - √ set data after editing (lambda?)
	// - √ specify native coordinate system

	auto updateLightLink = []( TablaApp& app )
	{
		// for some reason the recursive lambda capture (doing a capture in this function)
		// causes everything to explode, so just passing in a param.
		app.lightLinkDidChange();			
	};
	
	// - camera capture coords
	{
		mCameraPolyEditView = make_shared<PolyEditView>(
			PolyEditView(
				mApp,
				[this](){ return getPointsAsPoly(mApp.getCaptureProfileForPipeline().mCaptureCoords,4); },
				"undistorted"
				)
			);
		
		mCameraPolyEditView->setSetPolyFunc( [&]( const PolyLine2& poly ){
			setPointsFromPoly( mApp.getLightLink().getCaptureProfile().mCaptureCoords, 4, poly );
			updateLightLink(mApp);
		});
		
		mCameraPolyEditView->setMainImageView( mMainImageView );
		mCameraPolyEditView->getEditableInStages().push_back("undistorted");
		
		mViews.addView( mCameraPolyEditView );
	}

	// - projector mapping
	{
		// convert to input coords
		mProjPolyEditView = make_shared<PolyEditView>(
			PolyEditView(
				mApp,
				[&](){ return getPointsAsPoly(mApp.getLightLink().getProjectorProfile().mProjectorCoords,4); },
				"projector"
				)
		);
		
		mProjPolyEditView->setSetPolyFunc( [&]( const PolyLine2& poly ){
			setPointsFromPoly( mApp.getLightLink().getProjectorProfile().mProjectorCoords, 4, poly );
			updateLightLink(mApp);
		});
		
		mProjPolyEditView->setMainImageView( mMainImageView );
		mProjPolyEditView->getEditableInStages().push_back("projector");
		
		mViews.addView( mProjPolyEditView );
	}
	
	// - world boundaries
	// (for both camera + projector)
	{
		mWorldBoundsPolyEditView = make_shared<PolyEditView>(
			PolyEditView(
				mApp,
				[&](){ return getPointsAsPoly(mApp.getCaptureProfileForPipeline().mCaptureWorldSpaceCoords,4); },
				"world-boundaries"
				)
		);
		
		mWorldBoundsPolyEditView->setSetPolyFunc( [&]( const PolyLine2& poly ){
			setPointsFromPoly( mApp.getLightLink().getCaptureProfile().mCaptureWorldSpaceCoords  , 4, poly );
			setPointsFromPoly( mApp.getLightLink().getProjectorProfile().mProjectorWorldSpaceCoords, 4, poly );
			updateLightLink(mApp);
		});
		
		mWorldBoundsPolyEditView->setMainImageView( mMainImageView );
		mWorldBoundsPolyEditView->getEditableInStages().push_back("world-boundaries");
		
		mWorldBoundsPolyEditView->setConstrainToRect();
//			mWorldBoundsPolyEditView->setConstrainToRectWithAspectRatioOfPipelineStage("undistorted");
// this aspect ratio code works, except it's conceptually wrong headed! world bounds poly itself is what defines the aspect ratio ("clipped" size depends on world poly; and "undistorted" hasn't been clipped yet. :P). doh.
		mWorldBoundsPolyEditView->setQuantizeToUnit(1.f);
		mWorldBoundsPolyEditView->setCanEditVertexMask( vector<bool>{ false, false, true, false } );
		mWorldBoundsPolyEditView->setDrawSize(true);
		mWorldBoundsPolyEditView->setDrawPipelineStage("clipped");
		
		mViews.addView( mWorldBoundsPolyEditView );
	}
}

void TablaWindow::addMenus()
{
	// vector configuration ui
	{
		mVecEditView = make_shared<VectorEditView>();
		
		mViews.addView( mVecEditView );
	}

	// game library widget
	mGameLibraryView = std::make_shared<GameLibraryView>();
	mViews.addView( mGameLibraryView );
	
	// capture device
	mCaptureMenuView = std::make_shared<CaptureProfileMenuView>();
	mViews.addView( mCaptureMenuView );

	// layout
	layoutMenus();
}

void TablaWindow::draw()
{
	gl::ScopedViewport viewport( ivec2( 0, 0 ), getWindow()->getSize() );

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


	// ====== Window Space (UI) =====
	// baseline coordinate space
//	gl::setMatricesWindow( getWindowSize() );
	CameraOrtho cam(0.f, getWindow()->getWidth(), getWindow()->getHeight(), 0.f, kZNear, kZFar);
	gl::setMatrices(cam);	
	
	// views
	mViews.draw();
	
	// this, below, could become its own view
	// it would need a shared_ptr to MainImageView (no biggie)
	auto font = mApp.getFont();
	
	if ( mIsUIWindow && mApp.getParams().mDrawMouseDebugInfo )//&& getWindowBounds().contains(getMousePosInWindow()) )
	{
		const vec2 pt = getMousePosInWindow();
		
		// coordinates
		vec2   o[2] = { vec2(.5,1.5), vec2(0,0) };
		ColorA c[2] = { ColorA(0,0,0), ColorA(1,1,1) };
		
		for( int i=0; i<2; ++i )
		{
			gl::color( c[i] );
			font->drawString(
//				"Window: " + toString(pt) + "\t" +
				"Image: "  + toString( mMainImageView->windowToImage(pt) ) +
				"\tWorld: " + toString( mMainImageView->windowToWorld(pt) )
				, o[i]+vec2( 8.f, getWindowSize().y - font->getAscent()+font->getDescent()) ) ;
		}
	}
	
	if ( mIsUIWindow )
	{
		{
			string str = toString( (int)roundf(mApp.getCaptureFPS()) ) + " capture fps";
			
			vec2 size = font->measureString(str);
			
			//vec2 loc( vec2(getWindowSize().x - size.x - 8.f, getWindowSize().y - 8.f ) );
			vec2 loc( vec2(getWindowSize().x - size.x - 8.f, size.y + 8.f ) );

			gl::color(1,1,1,.8);
			font->drawString( str, loc );
		}
		
		const float targetFPS = mApp.getFrameRate();
		
		if ( targetFPS - mApp.getFPS() >= targetFPS*.1f )
		{
			string str = toString( (int)roundf(mApp.getFPS()) ) + " fps";
			
			vec2 size = font->measureString(str);
			
			vec2 loc( vec2(getWindowSize().x - size.x - 8.f, size.y + 24.f ) );

			gl::color(1,1,1,.8);
			font->drawString( str, loc );
		}
	}
	
	// draw contour debug info
	if (mApp.getParams().mDrawContourTree)
	{
		const float k = 16.f ;
		Color inc(0,0,0);
		
		auto color = [inc]( Color c )
		{
			gl::color(
				fmod( c.r + inc.r, 1.f ),
				fmod( c.g + inc.g, 1.f ),
				fmod( c.b + inc.b, 1.f ) );
		};
		
		auto contours = mApp.getVisionOutput().mContours;
		
		for ( size_t i=0; i<contours.size(); ++i, inc += .1f )
		{
			const auto& c = contours[i] ;
			
			Rectf rw = c.mBoundingRect ; // world space
			Rectf r  = Rectf(
				mMainImageView->worldToWindow(rw.getLowerLeft()),
				mMainImageView->worldToWindow(rw.getUpperRight()) );
				
			// rect in ui space
			r.y1 = ( c.mTreeDepth + 1 ) * k ;
			r.y2 = r.y1 + k ;
			
			if (c.mIsHole) color(Color(0,0,.3));
			else color(Color(0,.3,.4));
			
			gl::drawSolidRect(r) ;
			
			gl::color(.8,.8,.7);
			font->drawString( to_string(i) + " (" + to_string(c.mParent) + ")",
				r.getLowerLeft() + vec2(2,-font->getDescent()) ) ;
		}
	}

	// AV clacker
	if ( mApp.getAVClacker()>0.f )
	{
		gl::color(1,1,1,mApp.getAVClacker());
		gl::drawSolidRect( Rectf(vec2(0,0),getWindowSize()) );
	}
}

void TablaWindow::mouseDown( MouseEvent event )
{
	mMousePosInWindow = event.getPos();
	mViews.mouseDown(event);
}

void TablaWindow::mouseUp  ( MouseEvent event )
{
	mMousePosInWindow = event.getPos();
	mViews.mouseUp(event);
}

void TablaWindow::mouseMove( MouseEvent event )
{
	mMousePosInWindow = event.getPos();
	mViews.mouseMove(event);
}

void TablaWindow::mouseDrag( MouseEvent event )
{
	mMousePosInWindow = event.getPos();
	mViews.mouseDrag(event);
}

void TablaWindow::updateMainImageTransform()
{
	if (!mMainImageView) return;
	
	// get content bounds
	Rectf bounds;

	auto stage = mMainImageView->getPipelineStage();
	
	{
		// in terms of size...
		// this is a little weird, but it's basically historical code that needs to be
		// refactored.
		vec2 drawSize;
		
		if ( stage )
		{
			// should always be the case
			drawSize = stage->mImageSize;
		}
		else
		{
			// world by default. (this should never happen.)
			assert(mApp.getGameWorld());
			drawSize = Rectf( mApp.getGameWorld()->getWorldBoundsPoly().getPoints() ).getSize();
		}
		
		bounds = Rectf( 0, 0, drawSize.x, drawSize.y );
	}
	
	// what if it hasn't been made yet?
	mMainImageView->setBounds( bounds );
	
	// it fills the window
	Rectf windowRect = Rectf(0,0,mWindow->getSize().x,mWindow->getSize().y);
	Rectf margin = mMainImageView->getMargin();
	
	windowRect.x1 += margin.x1;
	windowRect.x2 -= margin.x2;
	windowRect.y1 += margin.y1;
	windowRect.y2 -= margin.y2;
	
	Rectf frame      = bounds.getCenteredFit( windowRect, true );
	
	mMainImageView->setFrame( frame );
}

void TablaWindow::updatePipelineViews()
{
	const auto stages = mApp.getPipeline().getStages();
	
	vec2 pos = vec2(1,1) * mApp.getParams().mConfigWindowPipelineGutter;
	const float left = pos.x;
	float top = pos.y;
	
	ViewRef lastView;
	Pipeline::StageRef lastStage;
	
	auto oldPipelineViews = mPipelineViews;
	mPipelineViews.clear();
	
	// cull views
	for( auto v : oldPipelineViews )
	{
		if ( !mApp.getParams().mDrawPipeline || !mApp.getPipeline().getStage(v->getName()) )
		{
			mViews.removeView(v);
		}
		else mPipelineViews.push_back(v);
	}
	
	// scan stages
	for( const auto &s : stages )
	{
		// view exists?
		ViewRef view = mViews.getViewByName(s->mName);
		
		// make a new one?
		if ( !view && mApp.getParams().mDrawPipeline )
		{
			PipelineStageView psv( s->mName );
			psv.setWorldDrawFunc( [&](){
				mApp.drawWorld( GameWorld::DrawType::UIPipelineThumb );
			});
			
			view = make_shared<PipelineStageView>(psv);
			
			view->setName(s->mName);
			
			mViews.addView(view);
			mPipelineViews.push_back(view);
		}
		
		// configure it
		if (view)
		{
			// compute size
			vec2 size = s->mImageSize * ((mApp.getParams().mConfigWindowPipelineWidth * s->mStyle.mScale) / s->mImageSize.x) ;
			
			// too tall?
			const float kMaxHeight = mApp.getParams().mConfigWindowPipelineWidth*2.f*s->mStyle.mScale;
			
			if (size.y > kMaxHeight) size = s->mImageSize * (kMaxHeight / s->mImageSize.y) ;
			
			vec2 usepos = pos;
			
			// do ortho layout
			if (lastView && lastStage
			 && lastStage->mStyle.mOrthoGroup >= 0
			 && lastStage->mStyle.mOrthoGroup == s->mStyle.mOrthoGroup )
			{
				usepos = lastView->getFrame().getUpperRight() + vec2(1,0) * mApp.getParams().mConfigWindowPipelineGutter;
			}
			
			view->setFrame ( Rectf(usepos, usepos + size) );
			view->setBounds( Rectf(vec2(0,0), s->mImageSize) );
			
			top = max( top, view->getFrame().y2 );
			
			// next pos
			pos = vec2( left, top + mApp.getParams().mConfigWindowPipelineGutter ) ;
			lastView = view;
			lastStage = s;
		}
	}
}

void TablaWindow::resize()
{
	updateMainImageTransform();
	
	layoutMenus();
	
	mViews.resize();
}

void TablaWindow::layoutMenus()
{
	const vec2 kGutter = vec2(8.f);
	vec2 lowerRight = Rectf(getWindow()->getBounds()).getLowerRight() - kGutter;
	vec2 size( 100, PopUpMenuView::getHeightWhenClosed() );

	if (mGameLibraryView)
	{
		mGameLibraryView->layout( Rectf( lowerRight - size, lowerRight ) );

		lowerRight = mGameLibraryView->getFrame().getLowerLeft() + vec2(-kGutter.x,0);
	}

	if (mCaptureMenuView)
	{
		mCaptureMenuView->layout( Rectf( lowerRight - size*vec2(2.f,1.f), lowerRight ) );
		
		lowerRight = mCaptureMenuView->getFrame().getLowerLeft() + vec2(-kGutter.x,0);
	}
	
	if (mVecEditView)
	{
		float r = PopUpMenuView::getHeightWhenClosed()/2;
		
		mVecEditView->setCenter( lowerRight + vec2(-kGutter.x,-r) );
		mVecEditView->setRadius(r);
	}
}

bool TablaWindow::isInteractingWithCalibrationPoly() const
{
	auto is = []( const std::shared_ptr<PolyEditView>& v ) -> bool
	{
		return v && (v->getHasRollover() || v->getHasMouseDown());
	};
	
	if ( is(mCameraPolyEditView)
	  || is(mProjPolyEditView)
	  || is(mWorldBoundsPolyEditView) ) return true;
	
	return false;
}
