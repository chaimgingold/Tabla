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

class PaperBounce3App;

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
	MainImageView( PaperBounce3App& app )
		: mApp(app)
//		: mPipeline(p)
//		, mGameWorld(w)
	{
	}

	void draw() override;
	void drawFrame() override; // none by default unless you set its color

	vec2 windowToImage( vec2 ); // maps mouse (screen) to drawn image coords
	vec2 windowToWorld( vec2 ); // maps mouse (screen) to world coordinates
	vec2 worldToWindow( vec2 );

	void mouseDown( MouseEvent ) override;
	
	void   setPipelineStageName( string n );
	string getPipelineStageName() const;
		// by default is empty to track Pipeline's query stage
		// setting it allows you to overload the query stage and lock it in place.
	
	const Pipeline::Stage* getPipelineStage() const;
	
	function<void(void)> mWorldDrawFunc; // this overloads use of mGameWorld.draw()
	// :P kinda lame.
	// this clunkiness is b/c app::drawWorld() handles all the contour drawing, debug info, etc...
	// for the time being.
	
	void	setFrameColor( ColorA c ) { mFrameColor=c; }
	ColorA	getFrameColor() { return mFrameColor; }
	
	void	setMargin( float m ) { mMargin=Rectf(1,1,1,1)*m; }
	void	setMargin( Rectf m ) { mMargin=m; }
	Rectf	getMargin() const { return mMargin; }
	
	void	setFont( gl::TextureFontRef f ) { mTextureFont=f; }
	
private:
	GameWorld* getGameWorld() const;
	Pipeline&  getPipeline () const;

	PaperBounce3App& mApp;
//	GameWorld&		mGameWorld;
//	Pipeline&		mPipeline;
	string			mStageName;

	ColorA			mFrameColor=ColorA(0,0,0,0); // none by default
	Rectf			mMargin; // we will place the image inset into this margin (css style margin for each side)
	
	gl::TextureFontRef mTextureFont;
};

class PolyEditView : public View
{
public:

	PolyEditView( Pipeline&, function<PolyLine2()>, string polyCoordSpace );
	
	void draw() override;
	bool pick( vec2 ) override;

	void mouseDown( MouseEvent ) override;
	void mouseUp  ( MouseEvent ) override;
	void mouseDrag( MouseEvent ) override;

	void setMainImageView( std::shared_ptr<MainImageView> ); // becomes the parent
	
	void setSetPolyFunc( function<void(const PolyLine2&)> f ) {mSetPolyFunc=f;}

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
	mat4 getImageToPolyTransform() const;
	
	Pipeline& mPipeline;

	string mPolyCoordSpace;
	
	function<PolyLine2()> mGetPolyFunc;
	function<void(const PolyLine2&)> mSetPolyFunc;
	
	PolyLine2 getPolyInImageSpace( bool withDrag=true ) const;
	PolyLine2 getPolyInPolySpace ( bool withDrag=true ) const;
	
	int mDragPointIndex=-1;
	vec2 mDragStartMousePos; // should really be in view manager; put it there when we do backlinks
	
	vec2 mDragStartPoint; // where poly point started (in poly space)
	vec2 mDragAtPoint; // where dragged poly point is (")
	
	bool mDoLiveUpdate=true;
	
	vector<string> mEditableInStages;
};

#endif /* PipelineStageView_hpp */
