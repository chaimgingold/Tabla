//
//  MainImageView.hpp
//  Tabla
//
//  Created by Chaim Gingold on 2/4/17.
//
//

#ifndef MainImageView_hpp
#define MainImageView_hpp

#include "Pipeline.h"
#include "GameWorld.h"
#include "View.h"
#include <string>

using namespace std;
using namespace ci;

class TablaApp;

// inherit PipelineStageView?
class MainImageView : public View
{
public:
	MainImageView( TablaApp& app )
		: mApp(app)
//		: mPipeline(p)
//		, mGameWorld(w)
	{
	}

	void setIsProjectorView( bool v ) { mIsProjectorView=v; }
	bool getIsProjectorView() { return mIsProjectorView; }
	
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
	
	const Pipeline::StageRef getPipelineStage() const;
	
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
	bool	mIsProjectorView=true;
	
	GameWorld* getGameWorld() const;
	const Pipeline&  getPipeline () const;

	TablaApp& mApp;
	string			mStageName;

	ColorA			mFrameColor=ColorA(0,0,0,0); // none by default
	Rectf			mMargin=Rectf(0,0,0,0); // we will place the image inset into this margin (css style margin for each side)
	
	gl::TextureFontRef mTextureFont;
};

#endif /* MainImageView_hpp */
