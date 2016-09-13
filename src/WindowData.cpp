//
//  WindowData.cpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 9/12/16.
//
//

#include "WindowData.h"
#include "PaperBounce3App.h"

WindowData::WindowData( WindowRef window, bool isUIWindow, PaperBounce3App& app )
	: mApp(app)
	, mWindow(window)
	, mIsUIWindow(isUIWindow)
{
	mMainImageView = make_shared<MainImageView>( MainImageView( mApp.mPipeline, mApp.mBallWorld ) );
	mMainImageView->mWorldDrawFunc = [&](){mApp.drawWorld();};
		// draw all the contours, etc... as well as the game world itself.
	mViews.addView(mMainImageView);
	
	// customize it
	if ( mIsUIWindow )
	{
		// margin
		Rectf margin = Rectf(1,1,1,1) * mApp.mConfigWindowMainImageMargin;
		margin.x1 = max( margin.x1, mApp.mConfigWindowPipelineWidth + mApp.mConfigWindowPipelineGutter*2.f );
		mMainImageView->setMargin( margin );
		mMainImageView->setFrameColor( ColorA(1,1,1,.35f) );
		mMainImageView->setFont(mApp.mTextureFont);
	}
	else
	{
		// lock it into this stage
		mMainImageView->setPipelineStageName("projector");
	}
	
	// poly editors
	if ( mIsUIWindow )
	{
		// TODO:
		// - colors per poly
		// - √ set data after editing (lambda?)
		// - √ specify native coordinate system

		auto getPointsAsPoly = []( const vec2* v, int n )
		{
			return PolyLine2( vector<vec2>(v,v+n) );
		};
		
		auto setPointsFromPoly = []( vec2* v, int n, PolyLine2 vv )
		{
			assert( vv.getPoints().size()==n );
			for( int i=0; i<n; ++i ) v[i] = vv.getPoints()[i];
		};
		
		// - camera capture coords
		{
			std::shared_ptr<PolyEditView> cameraPolyEditView = make_shared<PolyEditView>(
				PolyEditView(
					mApp.mPipeline,
					getPointsAsPoly(mApp.mLightLink.mCaptureCoords,4),
					"input"
					)
				);
			
			cameraPolyEditView->setPolyFunc( [&]( const PolyLine2& poly ){
				setPointsFromPoly( mApp.mLightLink.mCaptureCoords, 4, poly );
				mApp.mVision.setLightLink(mApp.mLightLink);
			});
			
			cameraPolyEditView->setMainImageView( mMainImageView );
			cameraPolyEditView->getEditableInStages().push_back("input");
			
			mViews.addView( cameraPolyEditView );
		}

		// - projector mapping
		{
			// convert to input coords
			std::shared_ptr<PolyEditView> projPolyEditView = make_shared<PolyEditView>(
				PolyEditView(
					mApp.mPipeline,
					getPointsAsPoly(mApp.mLightLink.mProjectorCoords,4),
					"projector"
					)
			);
			
			projPolyEditView->setPolyFunc( [&]( const PolyLine2& poly ){
				setPointsFromPoly( mApp.mLightLink.mProjectorCoords, 4, poly );
				mApp.mVision.setLightLink(mApp.mLightLink);
			});
			
			projPolyEditView->setMainImageView( mMainImageView );
			projPolyEditView->getEditableInStages().push_back("projector");
			
			mViews.addView( projPolyEditView );
		}
		
		// - world boundaries
		// (for both camera + projector)
		{
			std::shared_ptr<PolyEditView> projPolyEditView = make_shared<PolyEditView>(
				PolyEditView(
					mApp.mPipeline,
					getPointsAsPoly(mApp.mLightLink.mCaptureWorldSpaceCoords,4),
					"world-boundaries"
					)
			);
			
			projPolyEditView->setPolyFunc( [&]( const PolyLine2& poly ){
				setPointsFromPoly( mApp.mLightLink.mCaptureWorldSpaceCoords  , 4, poly );
				setPointsFromPoly( mApp.mLightLink.mProjectorWorldSpaceCoords, 4, poly );
				mApp.mVision.setLightLink(mApp.mLightLink);
			});
			
			projPolyEditView->setMainImageView( mMainImageView );
			projPolyEditView->getEditableInStages().push_back("world-boundaries");
			
			mViews.addView( projPolyEditView );
		}
	}
}

void WindowData::draw()
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


	// ====== Window Space (UI) =====
	// baseline coordinate space
	gl::setMatricesWindow( getWindowSize() );
	
	// views
	mViews.draw();
	
	// this, below, could become its own view
	// it would need a shared_ptr to MainImageView (no biggie)
	if ( mIsUIWindow && mApp.mDrawMouseDebugInfo )//&& getWindowBounds().contains(getMousePosInWindow()) )
	{
		const vec2 pt = getMousePosInWindow();
		
		// coordinates
		vec2   o[2] = { vec2(.5,1.5), vec2(0,0) };
		ColorA c[2] = { ColorA(0,0,0), ColorA(1,1,1) };
		
		for( int i=0; i<2; ++i )
		{
			gl::color( c[i] );
			mApp.mTextureFont->drawString(
				"Window: " + toString(pt) +
				"\tImage: "  + toString( mMainImageView->mouseToImage(pt) ) +
				"\tWorld: " + toString( mMainImageView->mouseToWorld(pt) )
				, o[i]+vec2( 8.f, getWindowSize().y - mApp.mTextureFont->getAscent()+mApp.mTextureFont->getDescent()) ) ;
		}
	}
}

void WindowData::mouseDown( MouseEvent event )
{
	mMousePosInWindow = event.getPos();
	mViews.mouseDown(event);
}

void WindowData::mouseUp  ( MouseEvent event )
{
	mMousePosInWindow = event.getPos();
	mViews.mouseUp(event);
}

void WindowData::mouseMove( MouseEvent event )
{
	mMousePosInWindow = event.getPos();
	mViews.mouseMove(event);
}

void WindowData::mouseDrag( MouseEvent event )
{
	mMousePosInWindow = event.getPos();
	mViews.mouseDrag(event);
}

void WindowData::updateMainImageTransform()
{
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
			// world by default.
			// (deprecated; not really sure what this even means anymore.)
			drawSize = mApp.getWorldSize();
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

