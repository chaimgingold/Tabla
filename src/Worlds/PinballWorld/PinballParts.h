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

inline int flipperTypeToIndex( PartType t )
{
	if (t==PartType::FlipperLeft) return 0;
	else if (t==PartType::FlipperRight) return 1;
	assert(0);
	return -1;
}

class Part
{
public:

	Part( PinballWorld& world ) : mWorld(world) {}
	
	virtual void draw();
	virtual void tick();
	
	bool isFlipper() const { return mType==PartType::FlipperLeft || mType==PartType::FlipperRight; }
	
	virtual void onBallCollide( Ball& ) {}
	
	PartType mType;
	vec2  mLoc;
	float mRadius=0.f;
	
	virtual PolyLine2 getCollisionPoly() const { return PolyLine2(); }
	// we could save some cpu by having a get/set and caching it internally, but who cares right now
	
	// contour origin info (for inter-frame coherency)
	// (when we do composite parts--multiple bumpers combined into one, we'll want to make this into a vector with a particular ordering
	// for easy comparisons)
	// might be most logical to just store the whole Contour, so a vector of Contours we are based on.
	vec2  mContourLoc;
	float mContourRadius=0.f;
	
	PinballWorld& mWorld;
};
typedef std::shared_ptr<Part> PartRef;
typedef vector<PartRef> PartVec;


class Flipper : public Part
{
public:
	Flipper( PinballWorld& world, vec2 pin, float contourRadius, PartType type );
	
	virtual void draw() override;
	virtual void tick() override;

	virtual PolyLine2 getCollisionPoly() const override;

	virtual void onBallCollide( Ball& ) override;
	
private:
	vec2 getAccelForBall( vec2 ) const;
	
	vec2  getTipLoc() const; // center of capsule, second point
	float mFlipperLength=0.f;

	float getCollisionFade() const;
	float mCollideTime = -10.f;
	
};

class Bumper : public Part
{
public:
	Bumper( PinballWorld& world, vec2 pin, float contourRadius, AdjSpace adjSpace );

	virtual void draw() override;
	virtual void tick() override;

	virtual void onBallCollide( Ball& ) override;

	virtual PolyLine2 getCollisionPoly() const override;
	
	float getCollisionFade() const;
	float mCollideTime = -10.f;

	ColorA mColor = ColorA(1,1,1,1);
	
};

} // namespace
#endif /* PinballParts_hpp */
