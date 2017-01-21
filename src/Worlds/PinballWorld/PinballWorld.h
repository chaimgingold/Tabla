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
#include "PinballVision.h"
#include "PinballInput.h"
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
	PinballVision mVision;
	
	// world orientation
	vec2 getUpVec() const { return mUpVec; }
	vec2 getLeftVec() const { return vec2(cross(vec3(mUpVec,0),vec3(0,0,1))); }
	vec2 getRightVec() const { return -getLeftVec(); }
	vec2 getDownVec() const { return -mUpVec; }
	vec2 getGravityVec() const { return getDownVec(); } // sugar
	
	float getTableDepth() const { return mView.m3dTableDepth; }
	void  getCullLine( vec2 v[2] ) const;
	float getBallRadius() const { return mBallDefaultRadius; }
	
	// state
	void sendGameEvent( GameEvent );
	
	bool isPaused() const { return mInput.isPaused(); }
	
	float getTime() const { return ci::app::getElapsedSeconds(); } // use this time so we can locally modulate it (eg slow down, pause, etc...)
	
	float getStrobeTime() const { return getTime(); }
	float getStrobe( float phase, float freq ) const;
	
	float getFlipperState( int side ) const { assert(side==0||side==1); return mFlipperState[side]; }
	float getFlipperAngularVel( int side ) const; // returns in radians per sim frame

	const PartVec& getParts() const { return mParts; }

	const PartCensus& getPartCensus() const { return mPartCensus; }

	cipd::PureDataNodeRef getPd() { return mPd; }

	vec2 getScreenShake() const { return mInput.isPaused() ? vec2() : mScreenShakeVec; }

	const ContourVec& getVisionContours() const { return mVisionContours ; }
	PinballVision::ContourType getVisionContourType( const Contour& ) const;
	
	// input callbacks
	void serveBallCheat();
	void serveBallIfNone();
	void doFlipperScreenShake() { addScreenShake(mFlipperScreenShake); }
	
private:

	void tickFlipperState();	
	float mFlipperState[2]; // left, right; 0..1	

	PinballInput mInput;
	
	friend class PinballView; // for getContoursFromParts
	
	FileWatch mFileWatch;
		
	// params
	vec2  mUpVec = vec2(0,1);
	float mGravity=0.1f;
	float mBallReclaimAreaHeight = 10.f;

	float mFlipperScreenShake=.05f;
	float mBallVelScreenShakeK=1.f;
	float mMaxScreenShake=2.f;
		
	// playfield layout
public:
	vec2  toPlayfieldSpace  ( vec2 p ) const { return vec2( dot(p,getRightVec()), dot(p,getUpVec()) ); }
	vec2  fromPlayfieldSpace( vec2 p ) const { return getRightVec() * p.x + getUpVec() * p.y ; }
		// could make this into two matrices
	vec2  normalizeToPlayfieldBBox( vec2 worldSpace ) const;
	vec2  fromNormalizedPlayfieldBBox( vec2 ) const;
	
private:
	Rectf calcPlayfieldBoundingBox() const; // min/max of all mVisionContours of type ContourType::Space 
	Rectf toPlayfieldBoundingBox ( const PolyLine2& ) const; // just for one poly
	
	Rectf mPlayfieldBoundingBox; // min/max of non-hole contours in playfield coordinate space (up/right vectors)
	
	float mPlayfieldBallReclaimY; // at what playfield y do we reclaim balls?
	float mPlayfieldBallReclaimX[2]; // left, right edges for drawing...
	
	void updatePlayfieldLayoutWithVisionContours();
	
	
	// parts
	typedef map<int,PartRef> ContourToPartMap;
	
	PartVec mParts;
	ContourToPartMap mContoursToParts;
	PartCensus mPartCensus;
	
	void getContoursFromParts( const PartVec&, ContourVec& contours, ContourToPartMap* c2pm=0 ) const; // for physics simulation
	
	PartRef findPartForContour( const Contour& ) const;
	PartRef findPartForContour( int contourIndex ) const;

	// parts + contours
	ContourVec mVisionContours;
	PinballVision::ContourTypes mVisionContourTypes;
	Contour contourFromPoly( PolyLine2 ) const; // area, radius, center, bounds, etc... is approximate
	void addContourToVec( Contour, ContourVec& ) const; // for accumulating physics contours from Parts
	
//	void rolloverTest();
//	bool isValidRolloverLoc( vec2 loc, float r, const PartVec& ) const;
	
	
	// --- Simulation ---
	void updateBallWorldContours();
	void processCollisions();
	void serveBall();
	void cullBalls(); // cull dropped balls
	
	void onGameEvent( GameEvent );
	
	
	
	
	// ---- Effects ----

	// screen shake
	float mScreenShakeValue=0.f;
	vec2 mScreenShakeVec;
	void addScreenShake( float intensity );
	void updateScreenShake();
	
	// party state
	enum class PartyType
	{
		None,
		Multiball,
		GameOver
	};
	
	float mPartyBegan = -1;
	PartyType mPartyType = PartyType::None;
	float getPartyProgress() const;
	vec2  getPartyLoc() const;
	bool  getIsInGameOverState() const { return getPartyProgress() > 0.f && mPartyType==PartyType::GameOver; }
	void  beginParty( PartyType type );
	int   mTargetCount=0; // for sound pitching
	
	
	// --- Sound Synthesis ---
	
	bool mSoundEnabled=true;
	
	void setupSynthesis() override;
	void updateSynthesis() override {};
	void shutdownSynthesis();
	
	void updateBallSynthesis();
};

} // namespace Pinball

#endif /* PinballWorld_hpp */
