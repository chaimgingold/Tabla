//
//  PinballWorld.hpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 12/5/16.
//
//

#ifndef PinballWorld_hpp
#define PinballWorld_hpp

#include "BallWorld.h"
#include "PureDataNode.h"
#include "GamepadManager.h"
#include "PinballParts.h"
#include "PinballView.h"
#include "FileWatch.h"

namespace Pinball
{

class PinballWorld : public BallWorld
{
public:

	PinballWorld();
	~PinballWorld();

	void setParams( XmlTree ) override;
	
	string getSystemName() const override { return "PinballWorld"; }

	void gameWillLoad() override;
	void update() override;
	void updateVision( const Vision::Output&, Pipeline& ) override;
	void prepareToDraw() override;
	void draw( DrawType ) override;
	
	void worldBoundsPolyDidChange() override;

	void keyDown( KeyEvent ) override;
	void keyUp( KeyEvent ) override;
	void mouseClick( vec2 ) override;

	PartParams mPartParams;
	PinballView mView;
	
public:

	// world orientation
	vec2 getUpVec() const { return mUpVec; }
	vec2 getLeftVec() const { return vec2(cross(vec3(mUpVec,0),vec3(0,0,1))); }
	vec2 getRightVec() const { return -getLeftVec(); }
	vec2 getDownVec() const { return -mUpVec; }
	vec2 getGravityVec() const { return getDownVec(); } // sugar
	
	float getTableDepth() const { return mView.m3dTableDepth; }
	void  getCullLine( vec2 v[2] ) const;
	
	// state
	void sendGameEvent( GameEvent );
	
	float getTime() const { return ci::app::getElapsedSeconds(); } // use this time so we can locally modulate it (eg slow down, pause, etc...)
	
	float getStrobeTime() const { return getTime(); }
	float getStrobe( float phase, float freq ) const;
	
	float getFlipperState( int side ) const { assert(side==0||side==1); return mFlipperState[side]; }
	float getFlipperAngularVel( int side ) const; // TODO: make radians per second

	const PartVec& getParts() const { return mParts; }

	cipd::PureDataNodeRef getPd() { return mPd; }

	vec2 getScreenShake() const { return mPauseBallWorld ? vec2() : mScreenShakeVec; }
	
private:
	
	friend class PinballView; // for getContoursFromParts
	
	FileWatch mFileWatch;
		
	// params
	vec2  mUpVec = vec2(0,1);
	float mGravity=0.1f;
	float mBallReclaimAreaHeight = 10.f;
		
	// playfield layout	
	vec2  toPlayfieldSpace  ( vec2 p ) const { return vec2( dot(p,getRightVec()), dot(p,getUpVec()) ); }
	vec2  fromPlayfieldSpace( vec2 p ) const { return getRightVec() * p.x + getUpVec() * p.y ; }
		// could make this into two matrices
	
	Rectf getPlayfieldBoundingBox( const ContourVec& ) const; // min/max of all points in playfield space
	Rectf toPlayfieldBoundingBox ( const PolyLine2& ) const; // just for one poly
	
	Rectf mPlayfieldBoundingBox; // min/max of non-hole contours in playfield coordinate space (up/right vectors)
	
	float mPlayfieldBallReclaimY; // at what playfield y do we reclaim balls?
	float mPlayfieldBallReclaimX[2]; // left, right edges for drawing...
	
	void updatePlayfieldLayout( const ContourVec& );
	
	
	// parts
	typedef map<int,PartRef> ContourToPartMap;
	
	PartVec mParts;
	ContourToPartMap mContoursToParts;

	void getContoursFromParts( const PartVec&, ContourVec& contours, ContourToPartMap* c2pm=0 ) const; // for physics simulation
	
	PartRef findPartForContour( const Contour& ) const;
	PartRef findPartForContour( int contourIndex ) const;
	
	void rolloverTest();
	bool isValidRolloverLoc( vec2 loc, float r, const PartVec& ) const;
	
	// simulation
	void updateBallWorldContours();
	void processCollisions();
	void serveBall();
	void cullBalls(); // cull dropped balls
	
	float mScreenShakeValue=0.f;
	vec2 mScreenShakeVec;
	void addScreenShake( float intensity );
	void updateScreenShake();
	
	void onGameEvent( GameEvent );
	
	// --- Vision ---
	//

public:
	// - inter-frame coherence params
	float mPartTrackLocMaxDist = 1.f;
	float mPartTrackRadiusMaxDist = .5f;
	float mDejitterContourMaxDist = 0.f;	

private:
	// - params
	float mPartMaxContourRadius = 5.f; // contour radius lt => part
	float mPartMaxContourAspectRatio = 2.f;
	float mHolePartMaxContourRadius = 2.f;
	float mFlipperDistToEdge = 10.f; // how close to the edge does a flipper appear?

	float mFlipperScreenShake=.05f;
	float mBallVelScreenShakeK=1.f;
	float mMaxScreenShake=2.f;
	
	// - do it
	PartVec getPartsFromContours( const ContourVector& ); // only reason this is non-const is b/c parts point to the world
	PartVec mergeOldAndNewParts( const PartVec& oldParts, const PartVec& newParts ) const;
	
public:
	AdjSpace getAdjacentSpace( const Contour*, vec2, const ContourVector& ) const ;
	AdjSpace getAdjacentSpace( vec2, const ContourVector& ) const ; // how much adjacent space is to the left, right?

	bool shouldContourBeAPart( const Contour&, const ContourVector& ) const;
	const ContourVec& getVisionContours() const { return mVisionContours ; }
	
private:
	ContourVec dejitterVisionContours( ContourVec in, ContourVec old ) const;
	
	// - state
	ContourVec mVisionContours;
	Contour contourFromPoly( PolyLine2 ) const; // area, radius, center, bounds, etc... is approximate
	void addContourToVec( Contour, ContourVec& ) const; // for accumulating physics contours from Parts
	
	
	// --- Input ---
	//
	
	// are flippers depressed
	bool  mIsFlipperDown[2]; // left, right
	float mFlipperState[2]; // left, right; 0..1
	
	void tickFlipperState();	
	
	bool mPauseBallWorld=false;
	
	// generic
	void setupControls();
	map<string,function<void()>> mInputToFunction; // maps input names to code handlers
	void processInputEvent( string name );
	
	// keyboard map
	map<char,string> mKeyToInput; // maps keystrokes to input names
	void processKeyEvent( KeyEvent, string suffix );

	// game pad
	GamepadManager mGamepadManager;
	map<unsigned int,string> mGamepadButtons;

	// game over state
	float mGameOverBegan = -1;
	float getGameOverProgress() const;
	void beginGameOver();

	// --- Sound Synthesis ---
	bool mSoundEnabled=true;

	
	void setupSynthesis() override;
	void updateSynthesis() override {};
	void shutdownSynthesis();
	
	void updateBallSynthesis();
};

} // namespace Pinball


class PinballWorldCartridge : public GameCartridge
{
public:
	virtual string getSystemName() const override { return "PinballWorld"; }

	virtual std::shared_ptr<GameWorld> load() const override
	{
		return std::make_shared<Pinball::PinballWorld>();
	}
};

#endif /* PinballWorld_hpp */
