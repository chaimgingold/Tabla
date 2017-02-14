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
#include "GameWorld.h"
#include "View.h"
#include <string>

using namespace std;
using namespace ci;

class TablaApp;

class PipelineStageView : public View
{
public:

	PipelineStageView( string stageName )
		: mStageName(stageName)
	{
	}

	void draw() override;
	
	void mouseDown( MouseEvent ) override;
	
	void drawFrame() override;

	typedef function<void(void)> fDraw;
	
	void setWorldDrawFunc( fDraw  f ) { mWorldDrawFunc=f; }
	
private:
	fDraw			mWorldDrawFunc;
	string			mStageName;
	
};

#endif /* PipelineStageView_hpp */