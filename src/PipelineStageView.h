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

class PipelineStageView : public View
{
public:

	PipelineStageView( Pipeline& p, string stageName )
		: mPipeline(p)
		, mStageName(stageName)
	{
	}

	void draw() override;
	
	void mouseDown( MouseEvent ) override;
	
	void drawFrame() override;

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
	MainImageView( Pipeline& p, GameWorld& w )
		: mPipeline(p)
		, mGameWorld(w)
	{
	}

	void draw() override;

	vec2 mouseToImage( vec2 ); // maps mouse (screen) to drawn image coords
	vec2 mouseToWorld( vec2 ); // maps mouse (screen) to world coordinates

	void mouseDown( MouseEvent ) override;
	
	function<void(void)> mWorldDrawFunc; // this overloads use of mGameWorld.draw()
	// :P kinda lame.
	// this clunkiness is b/c app::drawWorld() handles all the contour drawing, debug info, etc...
	// for the time being.
	
private:
	GameWorld&		mGameWorld;
	Pipeline&		mPipeline;
	
};

class PolyEditView : public View
{
public:

	PolyEditView( Pipeline&, PolyLine2 );
	
	void draw() override;
	bool pick( vec2 ) override;

	void mouseDown( MouseEvent ) override;
	void mouseUp  ( MouseEvent ) override;
	void mouseDrag( MouseEvent ) override;

private:
	
	int pickPoint( vec2 ) const;
	Rectf getPointControlRect( vec2 ) const;
	
	Pipeline& mPipeline;

	PolyLine2 mPoly;
	int mDragPointIndex=-1;
	vec2 mDragStartMousePos; // should really be in view manager; put it there when we do backlinks
	vec2 mDragStartPoint;
};

#endif /* PipelineStageView_hpp */
