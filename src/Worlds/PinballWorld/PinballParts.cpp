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

bool Part::getShouldMergeWithOldPart( const PartRef old ) const
{
	return old->getType() == getType() &&
		 distance( old->mContourLoc, mContourLoc ) < getWorld().mPartTrackLocMaxDist &&
		 fabs( old->mContourRadius - mContourRadius ) < getWorld().mPartTrackRadiusMaxDist
		;
	
}

Flipper::Flipper( PinballWorld& world, vec2 pin, float contourRadius, PartType type )
	: Part(world,type)
{
	mLoc=pin;

	const float kFlipperMinRadius = 1.f;

	mRadius = max( contourRadius, kFlipperMinRadius );
	mFlipperLength = constrain( mRadius * world.mFlipperRadiusToLengthScale, world.mFlipperMinLength, world.mFlipperMaxLength );
}

void Flipper::draw()
{
//	gl::color( lerp( getWorld().mFlipperColor, ColorA(1,1,1,1), powf(getCollisionFade(),3.f) ) );
	gl::color( getWorld().mFlipperColor );
	gl::drawSolid( getCollisionPoly() );
	
	if (getWorld().mDebugDrawFlipperAccelHairs)
	{
		PolyLine2 poly = getCollisionPoly();
		vec2 tip = getTipLoc();
		vec2 c = lerp(tip,mLoc,.5f);
		
		Rectf r(poly.getPoints());
		
		gl::color(1,0,0);
		auto hair = [&]( vec2 p )
		{
			p = lerp( c, p, 1.1f );
			vec2 v = getAccelForBall( p );
			gl::drawLine( p, p + v );
		};
		
		for( auto p : poly.getPoints() )
		{
			hair( lerp( c, p, 1.1f ) );
		}

		for( int i=0; i<poly.size(); ++i )
		{
			int j = (i+1) % poly.size();
			
			const int kk=10;
			for( int k=1; k<kk; ++k )
			{
				hair( lerp( poly.getPoints()[i], poly.getPoints()[j], (float)k/(float)kk ) );
			}
		}
		
		hair( vec2(lerp(r.x1,r.x2,.5),r.y2) );
		hair( vec2(lerp(r.x1,r.x2,.5),r.y1) );
	}
}

void Flipper::tick()
{
}

vec2
Flipper::getTipLoc() const
{
	// angle is degrees from down (aka gravity vec)
	float angle;
	{
		int   flipperIndex = flipperTypeToIndex(getType());
		float angleSign[2] = {-1.f,1.f};
		angle = angleSign[flipperIndex] * toRadians(45.f + getWorld().getFlipperState(flipperIndex)*90.f);
	}

	return glm::rotate( getWorld().getGravityVec() * mFlipperLength, angle) + mLoc;
}

PolyLine2 Flipper::getCollisionPoly() const
{
	vec2 c[2] = { mLoc, getTipLoc() };
	float r[2] = { mRadius, mRadius/2.f };
	
	return getWorld().getCapsulePoly(c,r);
}

float Flipper::getCollisionFade() const
{
	const float k = .5f;
	
	float t = 1.f - (getWorld().time() - mCollideTime) / k;
	
	t = constrain( t, 0.f, 1.f);
	
	return t;
}

void Flipper::onBallCollide( Ball& ball )
{
	mCollideTime = getWorld().time();
	
	ball.mAccel += getAccelForBall(ball.mLoc);
}

vec2 Flipper::getAccelForBall( vec2 p ) const
{
	// accelerate ball
	vec2 contact = closestPointOnPoly( p, getCollisionPoly() );
	vec2 surfaceNorm = p - contact;
	if (surfaceNorm != vec2(0,0))
	{
		surfaceNorm = normalize(surfaceNorm);
		
		float omega = getWorld().getFlipperAngularVel(flipperTypeToIndex(getType()));
		
		vec2 vs = -perp( contact-mLoc ) * omega;
		
		float surfaceVel = dot( vs, surfaceNorm );
		
		if ( surfaceVel > 0.f )
		{
			return 2.f * surfaceNorm * surfaceVel;
		}
	}
	
	return vec2(0,0);
}

Bumper::Bumper( PinballWorld& world, vec2 pin, float contourRadius, AdjSpace adjSpace )
	: Part(world,PartType::Bumper)
{
	mLoc=pin;

//	mColor = Color(Rand::randFloat(),Rand::randFloat(),Rand::randFloat());
	mColor = world.mBumperOuterColor;
	
	mRadius = min( max(contourRadius*world.mBumperContourRadiusScale,world.mBumperMinRadius),
					 min(adjSpace.mLeft,adjSpace.mRight)
					 );
}

void Bumper::draw()
{
	ColorA color = mColor;
	
	if ( Rand::randFloat() < powf(getCollisionFade(),2.f) )
	{
		color = Color(Rand::randFloat(),Rand::randFloat(),Rand::randFloat());
	}


//	gl::pushModelMatrix();
//	gl::translate( -mLoc );
//	gl::scale( vec2(1,1) * (1.f + .1f + getCollisionFade() ) );
//	gl::translate( mLoc );
	
	//gl::color(1,0,0);
	gl::color(color);
	gl::drawSolidCircle(mLoc, mRadius + .5 * powf(getCollisionFade(),2.f) );
//	gl::drawSolid( getCollisionPoly() );

//	gl::popModelMatrix();
	
	gl::color(getWorld().mBumperInnerColor);
	gl::drawSolidCircle(mLoc,mRadius/2.f);
}

void Bumper::tick()
{
}

PolyLine2 Bumper::getCollisionPoly() const
{
	return getWorld().getCirclePoly( mLoc, mRadius );
}

float Bumper::getCollisionFade() const
{
	const float k = .5f;
	
	float t = 1.f - (getWorld().time() - mCollideTime) / k;
	
	t = constrain( t, 0.f, 1.f);
	
	return t;
}

void Bumper::onBallCollide( Ball& ball )
{
//	mRadius /= 2.f;
//	mColor = Color(1,0,0);
//	ball.mColor = mColor;
	
	mCollideTime = getWorld().time();
	
	// kick ball
	vec2 v = ball.mLoc - mLoc;
	if (v==vec2(0,0)) v = randVec2();
	else v = normalize(v);
	
	ball.mAccel += v * getWorld().mBumperKickAccel ;
}

RolloverTarget::RolloverTarget( PinballWorld& world, vec2 pin, float radius )
	: Part(world,PartType::RolloverTargetOff)
	, mLoc(pin)
	, mRadius(radius)
{
	mColorOff = ColorA(0,.4,.1);
	mColorOn  = ColorA(.8,1.,.1);
}

void RolloverTarget::draw()
{
	gl::color( lerp(mColorOff,mColorOn,mLight) );
	gl::drawSolidCircle(mLoc, mRadius);
}

void RolloverTarget::tick()
{
	setZDepth( getWorld().getTableDepth() -.01f) ; // so we are responsive to hotloading
		// epsilon so we don't z-clip behind table back
		
	mLight = lerp( mLight, mIsLit ? 1.f : 0.f, .5f );
	
	const auto &balls = getWorld().getBalls();
	
	if (balls.empty()) setIsLit(false);
	else if ( !mIsLit )
	{
		for ( auto &b : balls )
		{
			if ( distance(b.mLoc,mLoc) < mRadius + b.mRadius )
			{
				setIsLit(true);
				break;
			}
		}
	}
}

void RolloverTarget::setIsLit( bool v )
{
	mIsLit=v;
	setType( v ? PartType::RolloverTargetOn : PartType::RolloverTargetOff );
}