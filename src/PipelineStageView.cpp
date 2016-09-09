//
//  PipelineStageView.cpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 9/7/16.
//
//

#include "PipelineStageView.h"

void PipelineStageView::draw()
{
	const Pipeline::Stage* stage = mPipeline.getStage(mStageName);		

	// image
	if ( stage && stage->mImage )
	{
		gl::color(1,1,1);
		gl::draw( stage->mImage, getBounds() );
	}
	else
	{
//		gl::color(0,0,0);
		gl::color(1,1,1);
		gl::drawSolidRect( getBounds() );
	}
	
	// world
	if ( mWorldDrawFunc )
	{
		if (stage)
		{
			gl::pushViewMatrix();
			gl::multViewMatrix( stage->mWorldToImage );
			// we need a pipeline stage so we know what transform to use
			// otherwise we'll just use existing matrix
		}
		
		mWorldDrawFunc();

		if (stage) gl::popViewMatrix();
	}
}

void PipelineStageView::mouseDown( MouseEvent )
{
	mPipeline.setQuery(mStageName);
}

void PipelineStageView::drawFrame()
{
	if ( mStageName == mPipeline.getQuery() ) gl::color(.7f,.3f,.5f);
	else gl::color(1,1,1,.5f);
	
	gl::drawStrokedRect( getFrame(), 2.f );
}

void MainImageView::draw()
{
	// vision pipeline image
	if ( mPipeline.getQueryStage() &&
		 mPipeline.getQueryStage()->mImage )
	{
		gl::color( 1, 1, 1 );
		
		gl::draw( mPipeline.getQueryStage()->mImage );
	}

//	if (0)
//	{
//		vec2 p = mouseToImage(mMousePos);
//		gl::color( 0., .5, .1 );
//		gl::drawSolidRect( Rectf(p,p+vec2(100,100)) );
//		gl::color( 1., .8, .1 );
//		gl::drawSolidRect( Rectf(p,p+vec2(10,10)) );
//		gl::color( .9, .8, .1 );
//		gl::drawSolidRect( Rectf(p,p+vec2(1,1)) );
//	}

	// ===== World space =====
	gl::pushViewMatrix();
	{
		if (mPipeline.getQueryStage())
		{
			// convert world coordinates to drawn texture coords
			gl::multViewMatrix( mPipeline.getQueryStage()->mWorldToImage );
		}
		
		if (mWorldDrawFunc) mWorldDrawFunc(); // overload it.
		else mGameWorld.draw();
	}
	gl::popViewMatrix();
}

void MainImageView::mouseDown( MouseEvent event )
{
	mGameWorld.mouseClick( mouseToWorld( event.getPos() ) ) ;
}

vec2 MainImageView::mouseToImage( vec2 p )
{
	// convert screen/window coordinates to image coords
	return parentToChild(p);
}

vec2 MainImageView::mouseToWorld( vec2 p )
{
	// convert image coordinates to world coords
	vec2 p2 = mouseToImage(p);
	
	if (mPipeline.getQueryStage())
	{
		p2 = vec2( mPipeline.getQueryStage()->mImageToWorld * vec4(p2,0,1) ) ;
	}
	
	return p2;
}