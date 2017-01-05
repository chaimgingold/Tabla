//
//  PinballParts.hpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 12/16/16.
//
//

#ifndef PinballParts_hpp
#define PinballParts_hpp

#include "BallWorld.h"

using namespace ci;
using namespace std;

namespace Pinball
{

class PinballWorld;
class Scene;

enum class GameEvent
{
	// game-wide
	ServeBall, // 0 => 1
	ServeMultiBall, // >0 => +1
	LostBall, // n => n-1
	LostLastMultiBall, // 1 => 0
	
	// part specific
	NewPart, // you are new
	
	// sent from parts
	ATargetTurnedOn
};

struct AdjSpace
{
	// amount of space at my left and right, from my contour's outer edge (centroid + my-width-left/right)
	float mLeft=0.f;
	float mRight=0.f;
	float mUp=0.f;
	float mDown=0.f;
	
	// width of my contour, from its centroid
	float mWidthLeft=0.f;
	float mWidthRight=0.f;
	float mWidthUp=0.f;
	float mWidthDown=0.f;
};

enum class PartType
{
	FlipperLeft,
	FlipperRight,
	Bumper,
	Target,
	Plunger
};

inline int flipperTypeToIndex( PartType t )
{
	if (t==PartType::FlipperLeft) return 0;
	else if (t==PartType::FlipperRight) return 1;
	assert(0);
	return -1;
}

class Part;
typedef std::shared_ptr<Part> PartRef;
typedef vector<PartRef> PartVec;

class PartParams
{
public:
	void set( const XmlTree& );
	
	float mBumperMinRadius = 5.f;
	float mBumperContourRadiusScale = 1.5f;
	float mBumperKickAccel = 1.f;
	ColorA mBumperOuterColor = ColorA(1,0,0,1);
	ColorA mBumperInnerColor = ColorA(1,.8,0,1);
	ColorA mBumperStrobeColor = ColorA(0,.8,1,1);
	ColorA mBumperOnColor = ColorA(0,.8,1,1);
	
	float mFlipperMinLength=5.f;
	float mFlipperMaxLength=10.f;
	float mFlipperRadiusToLengthScale=5.f;	
	ColorA mFlipperColor = ColorA(0,1,1,1);

	ColorA mTargetOnColor=Color(1,0,0);
	ColorA mTargetOffColor=Color(0,1,0);
	ColorA mTargetStrobeColor=ColorA(1,0,1);

	float mTargetRadius=1.f;
	float mTargetMinWallDist=1.f;
	bool  mTargetDynamicRadius=false;	
};

class Part
{
public:

	Part( PinballWorld& world, PartType type );
	
	virtual void draw(){}
	virtual void tick(){}
	virtual void addTo3dScene( Scene& ){}
	
	bool isFlipper() const { return mType==PartType::FlipperLeft || mType==PartType::FlipperRight; }
	
	virtual void onBallCollide( Ball& ) {}
	virtual void onGameEvent( GameEvent ) {}
	
	virtual PolyLine2 getCollisionPoly() const { return PolyLine2(); }
	// we could save some cpu by having a get/set and caching it internally, but who cares right now
	
	virtual vec2 getAdjSpaceOrigin() const { return mContourLoc; }
	virtual bool isValidLocForTarget( vec2 loc, float r ) const { return true; }
	
	PinballWorld& getWorld() const { return mWorld; }
	PartType getType() const { return mType; }
	
	// inter-frame coherency + persistence
	bool getShouldAlwaysPersist() const { return mShouldAlwaysPersist; }
	void setShouldAlwaysPersist( bool v ) { mShouldAlwaysPersist=v; }
	virtual bool getShouldMergeWithOldPart( const PartRef oldPart ) const;
	
	vec2  mContourLoc;
	float mContourRadius=0.f;
		// contour origin info
		// (when we do composite parts--multiple bumpers combined into one, we'll want to make this into a vector with a particular ordering
		// for easy comparisons)
		// might be most logical to just store the whole Contour, so a vector of Contours we are based on.

protected:
	void addExtrudedCollisionPolyToScene( Scene&, ColorA ) const;
	void setType( PartType t ) { mType=t; }

	void markCollision( float decay );
	float getCollisionFade() const;
	float getStrobe( float strobeFreqSlow, float strobeFreqFast ) const;
	void setStrobePhase( int nparts );
	
private:
	PartType mType;
	PinballWorld& mWorld;
	bool mShouldAlwaysPersist=false;

	float mStrobePhase=0.f;

	float mCollideTime = -10.f;
	float mCollideDecay=0.f;

};


class Flipper : public Part
{
public:
	Flipper( PinballWorld& world, vec2 pin, float contourRadius, PartType type );
	
	virtual void draw() override;
	virtual void tick() override;
	virtual void addTo3dScene( Scene& ) override;

	virtual PolyLine2 getCollisionPoly() const override;

	virtual void onBallCollide( Ball& ) override;

	virtual bool isValidLocForTarget( vec2 loc, float r ) const override {
		return distance(loc,mLoc) > r + mFlipperLength;
	}
	
private:
	vec2 getAccelForBall( vec2 ) const;

	vec2  mLoc;
	float mRadius=0.f;
	
	vec2  getTipLoc() const; // center of capsule, second point
	float mFlipperLength=0.f;
	
};

class Bumper : public Part
{
public:
	Bumper( PinballWorld& world, vec2 pin, float contourRadius, AdjSpace adjSpace );

	virtual void draw() override;
	virtual void tick() override;
	virtual void addTo3dScene( Scene& ) override;

	virtual void onBallCollide( Ball& ) override;

	virtual PolyLine2 getCollisionPoly() const override;

	virtual bool isValidLocForTarget( vec2 loc, float r ) const override {
		return distance(loc,mLoc) > r + mRadius;
	}

protected:
	void onGameEvent( GameEvent ) override;

private:
	ColorA getColor() const;
	
	float getDynamicRadius() const;
	
	vec2  mLoc;
	float mRadius=0.f;

	ColorA mColor = ColorA(1,1,1,1);
	ColorA mStrobeColor;
};

class Target : public Part
{
public:
	Target( PinballWorld& world, vec2 pin, float radius );

	virtual void draw() override;
	virtual void tick() override;

	virtual bool isValidLocForTarget( vec2 loc, float r ) const override {
		return distance(loc,mLoc) > r + mRadius;
	}

	virtual bool getShouldMergeWithOldPart( const PartRef oldPart ) const override;

	float mRadius;
	vec2  mLoc;
	PolyLine2 mContourPoly; // so we can strobe it!
	
	ColorA mColorOff, mColorOn, mColorStrobe;

protected:
	void onGameEvent( GameEvent ) override;
	
private:
	void setIsLit( bool );
	
	bool  mIsLit=false; // discrete goal
	float mLight=0.f; // continues, current anim state.
	
};

class Plunger : public Part
{
public:
	Plunger( PinballWorld& world, vec2 pin, float radius );

	virtual void draw() override;
	virtual void tick() override;

	float mRadius;
	vec2  mLoc;
	
};

} // namespace
#endif /* PinballParts_hpp */
