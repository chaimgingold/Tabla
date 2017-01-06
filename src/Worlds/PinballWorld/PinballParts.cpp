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
#include "PinballView.h"
#include "geom.h"
#include "cinder/rand.h"
#include "xml.h"

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace Pinball;

void PartParams::set( const XmlTree& xml )
{
	getXml(xml, "BumperMinRadius", mBumperMinRadius );
	getXml(xml, "BumperContourRadiusScale", mBumperContourRadiusScale );
	getXml(xml, "BumperKickAccel", mBumperKickAccel );

	getXml(xml, "BumperOuterColor",mBumperOuterColor);
	getXml(xml, "BumperInnerColor",mBumperInnerColor);
	getXml(xml, "BumperOnColor", mBumperOnColor);
	getXml(xml, "BumperStrobeColor",mBumperStrobeColor);
	
	getXml(xml, "FlipperMinLength",mFlipperMinLength);
	getXml(xml, "FlipperMaxLength",mFlipperMaxLength);
	getXml(xml, "FlipperRadiusToLengthScale",mFlipperRadiusToLengthScale);
	getXml(xml, "FlipperColor",mFlipperColor);	

	getXml(xml, "TargetRadius",mTargetRadius);
	getXml(xml, "TargetMinWallDist",mTargetMinWallDist);
	getXml(xml, "TargetOnColor",mTargetOnColor);
	getXml(xml, "TargetOffColor",mTargetOffColor);
	getXml(xml, "TargetOnStrobeColor",mTargetOnStrobeColor);
	getXml(xml, "TargetOffStrobeColor",mTargetOffStrobeColor);
	getXml(xml, "TargetDynamicRadius",mTargetDynamicRadius);
}

Part::Part( PinballWorld& world, PartType type )
	: mWorld(world), mType(type)
{
	setStrobePhase(2);
}

void Part::setStrobePhase( int nparts )
{
	if (nparts<2) mStrobePhase = 0.f;
	else mStrobePhase = randBool() ? 0.f : (1.f / (float)nparts);
}

bool Part::getShouldMergeWithOldPart( const PartRef old ) const
{
	return old->getType() == getType() &&
		 distance( old->mContourLoc, mContourLoc ) < getWorld().mPartTrackLocMaxDist &&
		 fabs( old->mContourRadius - mContourRadius ) < getWorld().mPartTrackRadiusMaxDist
		;
}

void Part::addExtrudedCollisionPolyToScene( Scene& s, ColorA c, const mat4* transform ) const
{
	float znear = 0.f;
	float zfar  = getWorld().getTableDepth();
	float extrudeDepth = zfar - znear ;
	
	PolyLine2 poly = getCollisionPoly();
	assert(poly.size()>0);
	
	std::function<Colorf(vec3)> posToColor = [&]( vec3 v ) -> Colorf
	{
		return c;
	};
	
	auto mesh = TriMesh::create(
		   geom::Extrude( getWorld().mView.polyToShape(poly), extrudeDepth ).caps(false).subdivisions( 1 )
		>> geom::Translate(0,0,extrudeDepth/2+znear)
		>> geom::ColorFromAttrib( geom::POSITION, posToColor ));
	
	s.mWalls.push_back( Scene::Obj(mesh,transform) );
}

void Part::markCollision( float decay )
{
	mCollideTime = getWorld().getTime();
	mCollideDecay = decay;
}

float Part::getCollisionFade() const
{
	float t = 1.f - (getWorld().getTime() - mCollideTime) / mCollideDecay;
	
	t = constrain( t, 0.f, 1.f);
	
	return t;
}

Flipper::Flipper( PinballWorld& world, vec2 pin, float contourRadius, PartType type )
	: Part(world,type)
{
	mLoc=pin;

	const float kFlipperMinRadius = 1.f;

	mRadius = max( contourRadius, kFlipperMinRadius );
	mFlipperLength = constrain(
		mRadius * world.mPartParams.mFlipperRadiusToLengthScale,
		world.mPartParams.mFlipperMinLength,
		world.mPartParams.mFlipperMaxLength );
}

void Flipper::draw()
{
//	gl::color( lerp( getWorld().mPartParams.mFlipperColor, ColorA(1,1,1,1), powf(getCollisionFade(),3.f) ) );
	gl::color( getWorld().mPartParams.mFlipperColor );
	gl::drawSolid( getCollisionPoly() );
	
	if (getWorld().mView.mDebugDrawFlipperAccelHairs)
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

void Flipper::addTo3dScene( Scene& s )
{
	addExtrudedCollisionPolyToScene(s, lerp(getWorld().mPartParams.mFlipperColor,ColorA(1,1,1,1),.5f) );
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
	
	return getWorld().mView.getCapsulePoly(c,r);
}

void Flipper::onBallCollide( Ball& ball )
{
	markCollision(.5f);
	
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
	mColor = world.mPartParams.mBumperOuterColor;
	mStrobeColor = world.mPartParams.mBumperStrobeColor;
	
	mRadius = min( max(contourRadius*world.mPartParams.mBumperContourRadiusScale,
					   world.mPartParams.mBumperMinRadius),
					 min(adjSpace.mLeft,adjSpace.mRight)
					 );
	
	setStrobePhase(4);
}

float Bumper::getDynamicRadius() const
{
	return mRadius + .5 * powf(getCollisionFade(),2.f);
}

ColorA Bumper::getColor() const
{
	return lerp(
		getCollisionFade() > 0.f ? getWorld().mPartParams.mBumperOnColor : mColor,
		mStrobeColor, getStrobe( 1.3f, .15f )
	);
}

void Bumper::draw()
{
//	gl::pushModelMatrix();
//	gl::translate( -mLoc );
//	gl::scale( vec2(1,1) * (1.f + .1f + getCollisionFade() ) );
//	gl::translate( mLoc );
	
	//gl::color(1,0,0);
	gl::color( getColor() );
	gl::drawSolidCircle(mLoc, getDynamicRadius() );
//	gl::drawSolid( getCollisionPoly() );

//	gl::popModelMatrix();
	
	{
		gl::ScopedModelMatrix trans;
		gl::translate(0,0,-.1f); // so we are above main bumper
		
		gl::color(getWorld().mPartParams.mBumperInnerColor);
		gl::drawSolidCircle(mLoc,mRadius/2.f);
	}
}

void Bumper::addTo3dScene( Scene& s )
{
	addExtrudedCollisionPolyToScene(s, lerp(getColor(),ColorA(1,1,1,1),.5f) );
}

void Bumper::tick()
{
}

void Bumper::onGameEvent( GameEvent e )
{
	switch(e)
	{
		case GameEvent::LostBall:
			markCollision(.75f);
			break;
			
		case GameEvent::LostLastMultiBall:
			markCollision(2.f);
			break;
			
		case GameEvent::NewPart:
			markCollision(.5f);
			break;
		
		default:break;
	}
}

PolyLine2 Bumper::getCollisionPoly() const
{
	return getWorld().mView.getCirclePoly( mLoc, getDynamicRadius() );
}

void Bumper::onBallCollide( Ball& ball )
{
	markCollision(.5f);
	
	// kick ball
	vec2 v = ball.mLoc - mLoc;
	if (v==vec2(0,0)) v = randVec2();
	else v = normalize(v);
	
	ball.mAccel += v * getWorld().mPartParams.mBumperKickAccel ;
	
	getWorld().getPd()->sendFloat("hit-bumper", length( getWorld().getDenoisedBallVel(ball)*10.f ) );
}

Target::Target( PinballWorld& world, vec2 triggerloc, vec2 triggervec, vec2 lightloc, float radius )
	: Part(world,PartType::Target)
	, mTriggerLoc(triggerloc)
	, mTriggerVec(triggervec)
	, mLightLoc(lightloc)
	, mRadius(radius)
{
	mColorOff = getWorld().mPartParams.mTargetOffColor;
	mColorOn  = getWorld().mPartParams.mTargetOnColor;
	mColorStrobe = getWorld().mPartParams.mTargetOffStrobeColor;

	setStrobePhase(2);
}

float Part::getStrobe( float strobeFreqSlow, float strobeFreqFast ) const
{
	float collideFade = getCollisionFade();
	
	return getWorld().getStrobe(
		mStrobePhase,
		collideFade > 0.f ? strobeFreqFast : strobeFreqSlow
	);
}

float Target::getMyStrobe() const
{
	return getStrobe( mIsLit ? .75f : 1.5f, .15f );
}

Color Target::getLightColor() const
{
	const float strobe = getMyStrobe();

	ColorA lightColor = lerp(
		lerp(mColorOff,mColorOn,mLight),
		lerp(mColorStrobe,getWorld().mPartParams.mTargetOnStrobeColor,mLight),
		strobe );
		
	return lightColor;
}

Color Target::getTriggerColor() const
{
	return getLightColor();
}

void Target::draw()
{
//	const float collideFade = getCollisionFade();
	
	Color triggerColor = getTriggerColor();
	
	// draw light on floor
	if (0)
	{
		Color lightColor = getLightColor();

		gl::pushModelView();
		gl::translate( 0, 0, getWorld().getTableDepth() -.01f); // epsilon so we don't z-clip behind table back
		
		gl::color(lightColor);
		gl::drawSolidCircle(mLightLoc, mRadius);

		gl::popModelView();
	}
	
	// highlight input contour
	if ( mContourPoly.size()>0 )
	{
		// we are at wrong z...
		// the objects should manage that themselves
		gl::ScopedDepth depthTest(false);
		gl::color(triggerColor);

		gl::drawSolid(mContourPoly);
	}
	
	//
	{
		gl::ScopedModelMatrix mat;
		gl::multModelMatrix(getAnimTransform());
		
		gl::color( triggerColor );
		gl::drawSolid( getCollisionPoly() );
	}
}

void Target::tick()
{
	mLight = lerp( mLight, mIsLit ? 1.f : 0.f, .5f );
	
	const auto &balls = getWorld().getBalls();
	
/*	if (balls.empty())
	{
		setIsLit(false);
	}
	else*/ if ( !mIsLit )
	{
		for ( auto &b : balls )
		{
			if ( distance(b.mLoc,mLightLoc) < mRadius + b.mRadius )
			{
				markCollision(.75f);
				setIsLit(true);
				break;
			}
		}
	}
}

float Target::getState() const
{
	if (mIsLit) return 1.f - getCollisionFade();
	else return 0.f;
}

PolyLine2 Target::getCollisionPoly() const
{
	PolyLine2 p;
	
	vec2 size(mRadius*2.f,mRadius*.5f);
	
	//    ^ mTriggerVec
	/* 0-----1
	   |     |   > xvec
	   3--x--2
	*/
	
	mat2 m( -perp(mTriggerVec), mTriggerVec );
	
	float xd = size.y;
	float v = xd * (1.f-getState());
	
	p.push_back( mTriggerLoc + m * vec2(-size.x/2,v) );
	p.push_back( mTriggerLoc + m * vec2( size.x/2,v) );
	p.push_back( mTriggerLoc + m * vec2( size.x/2,size.y+v) );
	p.push_back( mTriggerLoc + m * vec2(        0,size.y+v+xd) );
	p.push_back( mTriggerLoc + m * vec2(-size.x/2,size.y+v) );
	p.setClosed();
	
	return p;
}

mat4 Target::getAnimTransform() const
{
	mat4 transform;
	
	float strobe = getMyStrobe() * (1.f-getState());
	
	transform = glm::translate( vec3(0,0, lerp( 0.f, getWorld().getTableDepth()/2, strobe ) ) );
	
	return transform;
}

void Target::addTo3dScene( Scene& s )
{
	mat4 transform = getAnimTransform();
	
	addExtrudedCollisionPolyToScene(s, lerp(getTriggerColor(),Color(1,1,1),.5f), &transform );
}

void Target::setIsLit( bool v )
{
	if (v && !mIsLit) {
		getWorld().sendGameEvent( GameEvent::ATargetTurnedOn );
		vec2 param = getWorld().normalizeToPlayfieldBBox(mLightLoc);
		getWorld().getPd()->sendFloat("hit-rollover", (int)(param.y * 24.f) );
	}
	
	mIsLit=v;
//	setType( v ? PartType::TargetOn : PartType::TargetOff );
}

void Target::onGameEvent( GameEvent e )
{
	switch(e)
	{
		case GameEvent::NewGame:
			setIsLit(false);
			break;
		
//		case GameEvent::LostBall:
		case GameEvent::LostLastMultiBall:
			if (!mIsLit) markCollision(3.f);
			break;
			
		case GameEvent::NewPart:
			markCollision(.5f);
			break;
		
		default:break;
	}
}

bool Target::getShouldMergeWithOldPart( const PartRef old ) const
{
	if ( !Part::getShouldMergeWithOldPart(old) ) return false;
	
	if ( old->getType() == PartType::Target )
	{
		auto o = dynamic_cast<Target*>(old.get());
		assert(o);
		return distance( o->mTriggerLoc, mTriggerLoc ) < min( mRadius, o->mRadius ) ;
			// this constant needs to be bigger than mPartTrackLocMaxDist otherwise it jitters.
			// why? because as the angle of the contour jitters it shoots us off far away.
			// so we know the contour is the same, so we can be liberal here.
	}
	else return true;
}

Plunger::Plunger( PinballWorld& world, vec2 pin, float radius )
	: Part(world,PartType::Plunger)
	, mLoc(pin)
	, mRadius(radius)
{
}

void Plunger::draw()
{
	gl::color(1,1,1);
	gl::drawSolidCircle(mLoc, mRadius);
}

void Plunger::tick()
{
}
