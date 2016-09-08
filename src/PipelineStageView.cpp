//
//  PipelineStageView.cpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 9/7/16.
//
//

#include "PipelineStageView.h"

void MainImageView::draw()
{
	printf("hi\n");
	
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
		
		if (mWorldDrawFunc) mWorldDrawFunc();
		//drawWorld();
	}
	gl::popViewMatrix();
}