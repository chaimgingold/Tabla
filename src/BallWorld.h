//
//  Balls.hpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 8/4/16.
//
//

#ifndef Balls_hpp
#define Balls_hpp

#include <vector>
#include "cinder/gl/gl.h"
#include "cinder/Xml.h"
#include "cinder/Color.h"

#include "FileWatch.h"
#include "GameWorld.h"
#include "Contour.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class Ball {
	
public:
	vec2 mLoc ;
	vec2 mLastLoc ;
	vec2 mAccel ;
	
	float mRadius ;
	ColorAf mColor ;
	
	void setLoc( vec2 l ) { mLoc=mLastLoc=l; }
	void setVel( vec2 v ) { mLastLoc = mLoc - v ; }
	vec2 getVel() const { return mLoc - mLastLoc ; }
	
	void  setMass( float m ) { mMass = m ; }
	float getMass() const { return mMass ; }
	float getInvMass() const { return 1.f / getMass() ; }
	
	void noteSquashImpact( vec2 directionAndMagnitude )
	{
		if ( length(directionAndMagnitude) > length(mSquash) ) mSquash = directionAndMagnitude ;
	}

	vec2  mSquash ; // direction and magnitude
	bool  mCollideWithContours=true; // false: collide with inverse contours
	
private:
	float	mMass = 1.f ; // let's start by doing the right thing.

};


class BallWorld : public GameWorld
{
public:
	
	BallWorld();
	
	string getSystemName() const override { return "BallWorld"; }
	
	void setParams( XmlTree ) override;
	void updateVision( const Vision::Output& c, Pipeline& ) override { mContours = c.mContours; }
	
	void gameWillLoad() override; // make some balls by default
	void update() override;
	void prepareToDraw() override;
	void draw( DrawType ) override;
	
	Ball& newRandomBall( vec2 loc ); // returns it, too, if you want to modify it.
	void clearBalls() { mBalls.clear(); }
	
	float getBallDefaultRadius() const { return mBallDefaultRadius ; }
	
	vec2 resolveCollisionWithContours		( vec2 p, float r, const Ball* b=0 );
	vec2 resolveCollisionWithInverseContours( vec2 p, float r, const Ball* b=0 );
		// returns pinned version of point
		// public so we can show it with the mouse...
		// optional Ball* so we can record collisions (onBall*Collide())
		// not const because if b != 0 then we will note collisions and mutate state
	
	void mouseClick( vec2 p ) override { newRandomBall(p) ; }
	void keyDown( KeyEvent ) override;
	void drawMouseDebugInfo( vec2 ) override;
	
	vector<Ball>& getBalls() { return mBalls; }
	const vector<Ball>& getBalls() const { return mBalls; }
	
	ContourVector& getContours() { return mContours; }
	const ContourVector& getContours() const { return mContours; }
	
protected:
	virtual void onBallBallCollide			( const Ball&, const Ball& ){}
	virtual void onBallContourCollide		( const Ball&, const Contour& ){}
	virtual void onBallWorldBoundaryCollide	( const Ball& ){}
	// you probably want to just note this stuff and then transform Balls/Contours/etc... later
	// ideally BallWorld would just provide a list of collisions to others.
	// once we can uniquely id balls + contours with user data we'll switch to that model
	// and make these functions non-virtual and then they can do the recording.

	// params
	int		mDefaultNumBalls		= 5;
	float	mBallDefaultRadius		= 8.f *  .5f ;
	float	mBallDefaultMaxRadius	= 8.f * 4.f ;
	float	mBallMaxVel				= 8.f ;
	ColorAf mBallDefaultColor		= ColorAf::hex(0xC62D41);
	float   mBallContourImpactNormalVelImpulse = .0f; // for added excitement. if zero, then max vel matters less.
	
private:

	// simulation
	void updatePhysics();

	vec2 unlapEdge( vec2 p, float r, const Contour& poly, const Ball* b=0 );
	vec2 unlapHoles( vec2 p, float r, ContourKind kind, const Ball* b=0 );
	
	//
	vec2 resolveCollisionWithBalls		( vec2 p, float r, Ball* ignore=0, float correctionFraction=1.f ) const ;
		// simple pushes p out of overlapping balls.
		// but might take multiple iterations to respond to all of them
		// fraction is (0,1], how much of the collision correction to do.
	
	void resolveBallCollisions() ;

	ContourVector		mContours;
	vector<Ball>		mBalls ;

	// drawing
	void drawImmediate( bool lowPoly ) const;
	TriMeshRef getTriMeshForBalls() const;
	TriMeshRef mBallMesh; // Make into a Batch for even more performance (that we don't need)

	gl::GlslProgRef mCircleShader;
	
	// asset loading
	FileWatch mFileWatch; // hotload our shaders; in this class so when class dies all the callbacks expire.
	
} ;

class BallWorldCartridge : public GameCartridge
{
public:
	virtual string getSystemName() const override { return "BallWorld"; }

	virtual std::shared_ptr<GameWorld> load() const override
	{
		return std::make_shared<BallWorld>();
	}
};

#endif /* Balls_hpp */
