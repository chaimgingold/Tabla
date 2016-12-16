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

namespace Pinball
{

class PinballWorld;

struct AdjSpace
{
	// amount of space at my left and right, from my contour's outer edge (centroid + my-width-left/right)
	float mLeft=0.f;
	float mRight=0.f;
	
	// width of my contour, from its centroid
	float mWidthLeft=0.f;
	float mWidthRight=0.f;
};

enum class PartType
{
	FlipperLeft,
	FlipperRight,
	Bumper
};
	
class Part
{
public:

	Part( PinballWorld& world ) : mWorld(world) {}
	
	virtual void draw();
	virtual void tick();
	
	bool isFlipper() const { return mType==PartType::FlipperLeft || mType==PartType::FlipperRight; }
	
	ColorA mColor=ColorA(1,1,1,1);
	
	PartType  mType;
	vec2  mLoc;
	float mRadius=0.f;
	
	vec2  mFlipperLoc2; // second point of capsule that forms flipper
	float mFlipperLength=0.f;
	
	PolyLine2 mPoly;

	// contour origin info (for inter-frame coherency)
	// (when we do composite parts--multiple bumpers combined into one, we'll want to make this into a vector with a particular ordering
	// for easy comparisons)
	vec2  mContourLoc;
	float mContourRadius=0.f;
	
	AdjSpace mAdjSpace;
	
	PinballWorld& mWorld;
};
typedef std::shared_ptr<Part> PartRef;
typedef vector<PartRef> PartVec;


class Flipper : public Part
{
public:
	Flipper( PinballWorld& world, vec2 pin, float contourRadius, PartType type );
	
	virtual void draw();
	virtual void tick();
	
};

class Bumper : public Part
{
public:
	Bumper( PinballWorld& world, vec2 pin, float contourRadius, AdjSpace adjSpace );

	virtual void draw();
	virtual void tick();
	
};

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
	void draw( DrawType ) override;
	
	void worldBoundsPolyDidChange() override;

	void keyDown( KeyEvent ) override;

protected:
	virtual void onBallBallCollide   ( const Ball&, const Ball& ) override;
	virtual void onBallContourCollide( const Ball&, const Contour& ) override;
	virtual void onBallWorldBoundaryCollide	( const Ball& ) override;

public:

	// world orientation
	vec2 getUpVec() const { return mUpVec; }
	vec2 getLeftVec() const { return vec2(cross(vec3(mUpVec,0),vec3(0,0,1))); }
	vec2 getRightVec() const { return -getLeftVec(); }
	vec2 getGravityVec() const { return -mUpVec; }

	// geometry
	PolyLine2 getCirclePoly ( vec2 c, float r ) const;
	PolyLine2 getCapsulePoly( vec2 c[2], float r[2] ) const;

	// vision tuning params
	float mBumperMinRadius = 5.f;
	float mBumperContourRadiusScale = 1.5f;
	
	// state
	float getFlipperState( int side ) const { assert(side==0||side==1); return mFlipperState[side]; }
	
private:

	// params
	vec2  mUpVec = vec2(0,1);
	float mGravity=0.1f;
	float mPartMaxContourRadius = 5.f; // contour radius lt => part
	float mFlipperDistToEdge = 10.f; // how close to the edge does a flipper appear?
	float mBallReclaimAreaHeight = 10.f;

	int mCircleMinVerts=8;
	int mCircleMaxVerts=100;
	float mCircleVertsPerPerimCm=1.f;
	
	bool mDebugDrawAdjSpaceRays=false;
	bool mDebugDrawGeneratedContours=false;
	
	// more params, just for vision
	float mPartTrackLocMaxDist = 1.f;
	float mPartTrackRadiusMaxDist = .5f;
	
	// world layout
	vec2  toPlayfieldSpace  ( vec2 p ) const { return vec2( dot(p,getRightVec()), dot(p,getUpVec()) ); }
	vec2  fromPlayfieldSpace( vec2 p ) const { return getRightVec() * p.x + getUpVec() * p.y ; }
		// could make this into two matrices
	
	Rectf getPlayfieldBoundingBox( const ContourVec& ) const; // min/max of all points in playfield space
	Rectf toPlayfieldBoundingBox ( const PolyLine2& ) const; // just for one poly
	
	Rectf mPlayfieldBoundingBox; // min/max of non-hole contours in playfield coordinate space (up/right vectors)
	
	float mPlayfieldBallReclaimY; // at what playfield y do we reclaim balls?
	float mPlayfieldBallReclaimX[2]; // left, right edges for drawing...
	
	void updatePlayfieldLayout( const ContourVec& );
	
	void getContoursFromParts( const PartVec&, ContourVec& contours ) const; // for physics simulation
	
	PartVec mParts;
	void drawParts() const;
	
	// are flippers depressed
	bool  mIsFlipperDown[2]; // left, right
	float mFlipperState[2]; // left, right; 0..1
	
	void tickFlipperState();
	
	// simulation
	void serveBall();
	void cullBalls(); // cull dropped balls
	
	// drawing
	void drawAdjSpaceRays() const;
	
	// vision
	PartVec getPartsFromContours( const ContourVector& ); // only reason this is non-const is b/c parts point to the world
	PartVec mergeOldAndNewParts( const PartVec& oldParts, const PartVec& newParts ) const;
	AdjSpace getAdjacentLeftRightSpace( vec2, const ContourVector& ) const ; // how much adjacent space is to the left, right?
	
	// geometry
	int getNumCircleVerts( float r ) const;
	Contour contourFromPoly( PolyLine2 ) const; // area, radius, center, bounds, etc... is approximate
	void addContourToVec( Contour, ContourVec& ) const;
	
	// keymap (deprecated)
	map<char,string> mKeyToInput; // maps keystrokes to input names
	map<string,function<void()>> mInputToFunction; // maps input names to code handlers

	// game pad
	GamepadManager mGamepadManager;
	map<unsigned int,string> mGamepadButtons;
	map<string,function<void()>> mGamepadFunctions;
	
	// synthesis
	cipd::PureDataNodeRef	mPureDataNode;	// synth engine
	cipd::PatchRef			mPatch;			// pong patch
	
	void setupSynthesis();
	void shutdownSynthesis();

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
