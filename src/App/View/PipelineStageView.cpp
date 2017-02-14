//
//  PipelineStageView.cpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 9/7/16.
//
//

#include "PipelineStageView.h"
#include "TablaApp.h"

void PipelineStageView::draw()
{
	// scissor us so we only draw into our little frame
	// this will fail to work right if we are nested in other transforms
	gl::ScopedScissor scissor( getScissorLowerLeft(), getScissorSize() );
	
	const Pipeline::StageRef stage = TablaApp::get()->getPipeline().getStage(mStageName);

	bool drawWorld = true;
	
	// image
	if ( stage && stage->getGLImage() )
	{
		gl::color(1,1,1);
		gl::draw( stage->getGLImage(), getBounds() );
	}
	else if ( stage && stage->mImageCubeMapGL )
	{
		drawWorld = false;
		gl::color(1,1,1);
		gl::drawHorizontalCross( stage->mImageCubeMapGL, getBounds() );
	}
	else
	{
//		gl::color(0,0,0);
		gl::color(1,1,1);
		gl::drawSolidRect( getBounds() );
	}
	
	// world
	if ( mWorldDrawFunc && drawWorld )
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
	TablaApp::get()->selectPipelineStage(mStageName);
}

void PipelineStageView::drawFrame()
{
	if ( mStageName == TablaApp::get()->getPipelineStageSelection() ) gl::color(.7f,.3f,.5f);
	else if ( getHasRollover() ) gl::color(1,1,1,1.f);
	else gl::color(1,1,1,.5f);
	
	gl::drawStrokedRect( getFrame(), 2.f );
}