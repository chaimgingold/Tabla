//
//  WindowData.cpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 9/12/16.
//
//

#include "WindowData.h"
#include "PaperBounce3App.h"
#include "geom.h" // getPointsAsPoly

WindowData::WindowData( WindowRef window, bool isUIWindow, PaperBounce3App& app )
	: mApp(app)
	, mWindow(window)
	, mIsUIWindow(isUIWindow)
{
	mMainImageView = make_shared<MainImageView>( MainImageView(mApp) );
	mMainImageView->mWorldDrawFunc = [&](){mApp.drawWorld(true);};
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

		auto updateLightLink = []( PaperBounce3App& app )
		{
			// for some reason the recursive lambda capture (doing a capture in this function)
			// causes everything to explode, so just passing in a param.
			app.lightLinkDidChange(app.mLightLink);
			
			XmlTree lightLinkXml = app.mLightLink.getParams();
			
			lightLinkXml.write( writeFile(app.getUserLightLinkFilePath()) );
			// this will trigger a reload, but that shouldn't matter.
		};
		
		// - camera capture coords
		{
			std::shared_ptr<PolyEditView> cameraPolyEditView = make_shared<PolyEditView>(
				PolyEditView(
					mApp.mPipeline,
					[&](){ return getPointsAsPoly(mApp.mLightLink.mCaptureCoords,4); },
					"input"
					)
				);
			
			cameraPolyEditView->setSetPolyFunc( [&]( const PolyLine2& poly ){
				setPointsFromPoly( mApp.mLightLink.mCaptureCoords, 4, poly );
				updateLightLink(mApp);
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
					[&](){ return getPointsAsPoly(mApp.mLightLink.mProjectorCoords,4); },
					"projector"
					)
			);
			
			projPolyEditView->setSetPolyFunc( [&]( const PolyLine2& poly ){
				setPointsFromPoly( mApp.mLightLink.mProjectorCoords, 4, poly );
				updateLightLink(mApp);
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
					[&](){ return getPointsAsPoly(mApp.mLightLink.mCaptureWorldSpaceCoords,4); },
					"world-boundaries"
					)
			);
			
			projPolyEditView->setSetPolyFunc( [&]( const PolyLine2& poly ){
				setPointsFromPoly( mApp.mLightLink.mCaptureWorldSpaceCoords  , 4, poly );
				setPointsFromPoly( mApp.mLightLink.mProjectorWorldSpaceCoords, 4, poly );
				updateLightLink(mApp);
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
//				"Window: " + toString(pt) + "\t" +
				"Image: "  + toString( mMainImageView->windowToImage(pt) ) +
				"\tWorld: " + toString( mMainImageView->windowToWorld(pt) )
				, o[i]+vec2( 8.f, getWindowSize().y - mApp.mTextureFont->getAscent()+mApp.mTextureFont->getDescent()) ) ;
		}
	}
	
	// draw contour debug info
	if (mApp.mDrawContourTree)
	{
		const float k = 16.f ;
		
		for ( size_t i=0; i<mApp.mContours.size(); ++i )
		{
			const auto& c = mApp.mContours[i] ;
			
			Rectf rw = c.mBoundingRect ; // world space
			Rectf r  = Rectf(
				mMainImageView->worldToWindow(rw.getLowerLeft()),
				mMainImageView->worldToWindow(rw.getUpperRight()) );
				
			// rect in ui space
			r.y1 = ( c.mTreeDepth + 1 ) * k ;
			r.y2 = r.y1 + k ;
			
			if (c.mIsHole) gl::color(0,0,.3);
			else gl::color(0,.3,.4);
			
			gl::drawSolidRect(r) ;
			
			gl::color(.8,.8,.7);
			mApp.mTextureFont->drawString( to_string(i) + " (" + to_string(c.mParent) + ")",
				r.getLowerLeft() + vec2(2,-mApp.mTextureFont->getDescent()) ) ;
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

void WindowData::updatePipelineViews()
{
	const auto stages = mApp.mPipeline.getStages();
	
	vec2 pos = vec2(1,1) * mApp.mConfigWindowPipelineGutter;
	const float left = pos.x;
	
	ViewRef lastView;
	bool wasLastViewOrtho=false;
	
	auto oldPipelineViews = mPipelineViews;
	mPipelineViews.clear();
	
	// cull views
	for( auto v : oldPipelineViews )
	{
		if ( !mApp.mDrawPipeline || !mApp.mPipeline.getStage(v->getName()) )
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
		if ( !view && mApp.mDrawPipeline )
		{
			PipelineStageView psv( mApp.mPipeline, s->mName );
			psv.setWorldDrawFunc( [&](){ mApp.drawWorld(false); } );
			
			view = make_shared<PipelineStageView>(psv);
			
			view->setName(s->mName);
			
			mViews.addView(view);
			mPipelineViews.push_back(view);
		}
		
		// configure it
		if (view)
		{
			// update its location
			vec2 size = s->mImageSize;
			
			size *= (mApp.mConfigWindowPipelineWidth * s->mLayoutHintScale) / size.x ;
			
			vec2 usepos = pos;
			
			// do ortho layout
			if (lastView && wasLastViewOrtho && s->mLayoutHintOrtho)
			{
				usepos = lastView->getFrame().getUpperRight() + vec2(1,0) * mApp.mConfigWindowPipelineGutter;
			}
			
			view->setFrame ( Rectf(usepos, usepos + size) );
			view->setBounds( Rectf(vec2(0,0), s->mImageSize) );
			
			lastView = view;
			wasLastViewOrtho = s->mLayoutHintOrtho;
			
			// next pos
			pos = vec2( left, view->getFrame().y2 + mApp.mConfigWindowPipelineGutter ) ;
		}
	}
}