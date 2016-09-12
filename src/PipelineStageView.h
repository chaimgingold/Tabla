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
	
	string getPipelineStageName() const { return mPipeline.getQuery(); }
	
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

	PolyEditView( Pipeline&, PolyLine2, string polyCoordSpace );
	
	void draw() override;
	bool pick( vec2 ) override;

	void mouseDown( MouseEvent ) override;
	void mouseUp  ( MouseEvent ) override;
	void mouseDrag( MouseEvent ) override;

	void setMainImageView( std::shared_ptr<MainImageView> ); // becomes the parent
	
	void setPolyFunc( function<void(const PolyLine2&)> f ) {mPolyFunc=f;}

	void setDoLiveUpdate( bool v ) { mDoLiveUpdate=v; }
	bool getDoLiveUpdate() const { return true; }

	// what stages are we editable in?
	// by default (if empty), then all
	vector<string>& getEditableInStages() { return mEditableInStages; }
	const vector<string>& getEditableInStages() const { return mEditableInStages; }
	
	bool isEditable() const ;
	
private:
	
	std::shared_ptr<MainImageView> mMainImageView;
	
	int pickPoint( vec2 ) const; // image coord space
	Rectf getPointControlRect( vec2 ) const; // image coord space

	mat4 getPolyToImageTransform() const;
	mat4 getImagetoPolyTransform() const;
	
	Pipeline& mPipeline;

	string mPolyCoordSpace;
	
	function<void(const PolyLine2&)> mPolyFunc;
	
	PolyLine2 mPoly; // poly coord space
	PolyLine2 getPolyInImageSpace() const;
	PolyLine2 getPolyInPolySpace() const { return mPoly; }
	
	int mDragPointIndex=-1;
	vec2 mDragStartMousePos; // should really be in view manager; put it there when we do backlinks
	vec2 mDragStartPoint;
	
	bool mDoLiveUpdate=true;
	
	vector<string> mEditableInStages;
};

#endif /* PipelineStageView_hpp */
