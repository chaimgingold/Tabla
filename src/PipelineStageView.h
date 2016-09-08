//
//  PipelineStageView.hpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 9/7/16.
//
//

#ifndef PipelineStageView_hpp
#define PipelineStageView_hpp

#include "Pipeline.h"
#include "View.h"
#include <string>

using namespace std;
using namespace ci;

class PipelineStageView : public View
{
public:

	PipelineStageView( Pipeline& p, string stageName )
		: mPipeline(p)
		, mStageName(stageName)
	{
	}

	void draw() override
	{
		const Pipeline::Stage* stage = mPipeline.getStage(mStageName);		

		// image
		gl::color(1,1,1);
		
		if ( stage && stage->mImage )
		{
			gl::draw( stage->mImage, getBounds() );
		}
		else
		{
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
	
	void mouseDown( MouseEvent ) override
	{
		mPipeline.setQuery(mStageName);
	}
	
	void drawFrame() override
	{
		if ( mStageName == mPipeline.getQuery() ) gl::color(.7f,.3f,.5f);
		else gl::color(1,1,1,.5f);
		
		gl::drawStrokedRect( getFrame(), 2.f );
	}

	typedef function<void(void)> fDraw;
	
	void setWorldDrawFunc( fDraw  f ) { mWorldDrawFunc=f; }
	
private:
	fDraw			mWorldDrawFunc;
	Pipeline&		mPipeline; // not const so we can set the stage; but that state should move into app.
	string			mStageName;
	
};

// inherit PipelineStageView?
class MainImageView : public View
{
public:
	MainImageView( Pipeline& p )
		: mPipeline(p)
	{
		printf("yar");
	}

	void draw() override;
	
	typedef function<void(void)> fDraw;
	
	void setWorldDrawFunc( fDraw  f ) { mWorldDrawFunc=f; }
	
private:
	fDraw			mWorldDrawFunc;
	Pipeline&		mPipeline;
	
};


#endif /* PipelineStageView_hpp */
