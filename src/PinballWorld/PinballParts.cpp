//
//  PinballParts.cpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 12/16/16.
//
//

#include "PinballParts.h"

#include "glm/glm.hpp"
#include "PinballWorld.h"
#include "geom.h"
#include "cinder/rand.h"

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace Pinball;

void Part::draw()
{
	
}

void Part::tick()
{
	
}

Flipper::Flipper( PinballWorld& world, vec2 pin, float contourRadius, PartType type )
	: Part(world)
{
	mType=type;
	mLoc=pin;

	const float kFlipperMinRadius = 1.f;

	mRadius = max( contourRadius, kFlipperMinRadius );
	mFlipperLength = constrain( mRadius * world.mFlipperRadiusToLengthScale, world.mFlipperMinLength, world.mFlipperMaxLength );
}

void Flipper::draw()
{
	gl::color( ColorA(0,1,1,1) );
	gl::drawSolid( getCollisionPoly() );
}

void Flipper::tick()
{
}

PolyLine2 Flipper::getCollisionPoly() const
{
	// angle is degrees from down (aka gravity vec)
	float angle;
	{
		int   flipperIndex = mType==PartType::FlipperLeft ? 0 : 1;
		float angleSign[2] = {-1.f,1.f};
		angle = angleSign[flipperIndex] * toRadians(45.f + mWorld.getFlipperState(flipperIndex)*90.f);
	}

	vec2 flipperLoc2 = glm::rotate( mWorld.getGravityVec() * mFlipperLength, angle) + mLoc;
	
	vec2 c[2] = { mLoc, flipperLoc2 };
	float r[2] = { mRadius, mRadius/2.f };
	
	return mWorld.getCapsulePoly(c,r);
}

Bumper::Bumper( PinballWorld& world, vec2 pin, float contourRadius, AdjSpace adjSpace )
	: Part(world)
{
	mType=PartType::Bumper;
	mLoc=pin;

	mColor = Color(Rand::randFloat(),Rand::randFloat(),Rand::randFloat());

	mRadius = min( max(contourRadius*world.mBumperContourRadiusScale,world.mBumperMinRadius),
					 min(adjSpace.mLeft,adjSpace.mRight)
					 );
}

void Bumper::draw()
{
	//gl::color(1,0,0);
	gl::color(mColor);
//	gl::drawSolidCircle(p->mLoc,p->mRadius);
	gl::drawSolid( getCollisionPoly() );
	gl::color(1,.8,0);
	gl::drawSolidCircle(mLoc,mRadius/2.f);
}

void Bumper::tick()
{
}

PolyLine2 Bumper::getCollisionPoly() const
{
	return mWorld.getCirclePoly( mLoc, mRadius );
}

void Bumper::onBallCollide( const Ball& )
{
//	mRadius /= 2.f;
	mColor = Color(Rand::randFloat(),Rand::randFloat(),Rand::randFloat());
//	mColor = Color(1,0,0);
}