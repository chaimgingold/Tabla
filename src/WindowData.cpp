//
//  WindowData.cpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 9/12/16.
//
//

#include "WindowData.h"
#include "PaperBounce3App.h"

WindowData::WindowData( bool isUIWindow, PaperBounce3App& app )
	: mApp(app)
	, mIsUIWindow(isUIWindow)
{
	mMainImageView = make_shared<MainImageView>( MainImageView( mApp.mPipeline, mApp.mBallWorld ) );
	mMainImageView->mWorldDrawFunc = [&](){mApp.drawWorld();};
		// draw all the contours, etc... as well as the game world itself.
	mViews.addView(mMainImageView);
	
	// poly editors
	if ( mIsUIWindow )
	{
		// TODO:
		// - colors per poly
		// - √ set data after editing (lambda?)
		// - √ specify native coordinate system

		// - camera capture coords
		{
			std::shared_ptr<PolyEditView> cameraPolyEditView = make_shared<PolyEditView>(
				PolyEditView(
					mApp.mPipeline,
					PolyLine2(vector<vec2>(mApp.mLightLink.mCaptureCoords,mApp.mLightLink.mCaptureCoords+4)),
					"input"
					)
				);
			
			cameraPolyEditView->setPolyFunc( [&]( const PolyLine2& poly ){
				assert( poly.getPoints().size()==4 );
				for( int i=0; i<4; ++i ) mApp.mLightLink.mCaptureCoords[i] = poly.getPoints()[i];
				mApp.mVision.setLightLink(mApp.mLightLink);
			});
			
			cameraPolyEditView->setMainImageView( mMainImageView );
			cameraPolyEditView->getEditableInStages().push_back("input");
			
			mViews.addView( cameraPolyEditView );
		}

		// - projector mapping
		if (1)
		{
			PolyLine2 poly(vector<vec2>(mApp.mLightLink.mProjectorCoords,mApp.mLightLink.mProjectorCoords+4));
			
			// convert to input coords
			std::shared_ptr<PolyEditView> projPolyEditView = make_shared<PolyEditView>(
				PolyEditView(
					mApp.mPipeline,
					PolyLine2(vector<vec2>(mApp.mLightLink.mProjectorCoords,mApp.mLightLink.mProjectorCoords+4)),
					"projector"
					)
			);
			
			projPolyEditView->setPolyFunc( [&]( const PolyLine2& poly ){
				assert( poly.getPoints().size()==4 );
				for( int i=0; i<4; ++i ) mApp.mLightLink.mProjectorCoords[i] = poly.getPoints()[i];
				mApp.mVision.setLightLink(mApp.mLightLink);
			});
			
			projPolyEditView->setMainImageView( mMainImageView );
			projPolyEditView->getEditableInStages().push_back("projector");
			
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

	{
		// in terms of size...
		// this is a little weird, but it's basically historical code that needs to be
		// refactored.
		vec2 drawSize;
		
		if ( mApp.mPipeline.getQueryStage() )
		{
			// should always be the case
			drawSize = mApp.mPipeline.getQueryStage()->mImageSize;
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
	if (mMainImageView)
	{
		mMainImageView->setBounds( bounds );
		
		// it fills the window
		const Rectf windowRect = Rectf(0,0,getWindowSize().x,getWindowSize().y);
		const Rectf frame      = bounds.getCenteredFit( windowRect, true );
		
		mMainImageView->setFrame( frame );
	}
}

