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
#include <boost/circular_buffer.hpp>

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
	
	boost::circular_buffer<vec2> mHistory;
	
private:
	float	mMass = 1.f ; // let's start by doing the right thing.

};


class BallWorld : public GameWorld
{
public:
	
	BallWorld();
	
	string getSystemName() const override { return "BallWorld"; }
	
	void setParams( XmlTree ) override;
	void updateVision( const Vision::Output& c, Pipeline& ) override { setContours(c.mContours); }
	
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
	
	// tracking collisions
	struct BallBallCollision {
	public:
		BallBallCollision( int a=-1, int b=-1 ) { mBallIndex[0]=a; mBallIndex[1]=b; }
		int mBallIndex[2];
	};
	typedef vector<BallBallCollision> BallBallCollisionVec;
	
	struct BallContourCollision {
	public:
		BallContourCollision( int a=-1, int b=-1 ) { mBallIndex=a; mContourIndex=b; }
		int mBallIndex;
		int mContourIndex;
	};
	typedef vector<BallContourCollision> BallContourCollisionVec;

	struct BallWorldCollision {
	public:
		BallWorldCollision( int a=-1 ) { mBallIndex=a; }
		int mBallIndex;
	};
	typedef vector<BallWorldCollision> BallWorldCollisionVec;

	const BallBallCollisionVec&		getBallBallCollisions() const { return mBallBallCollisions; }
	const BallContourCollisionVec&	getBallContourCollisions() const { return mBallContourCollisions; }
	const BallWorldCollisionVec&	getBallWorldCollisions() const { return mBallWorldCollisions; }
	
	int getNumIntegrationSteps() const { return mNumIntegrationSteps; }
	
protected:

	void drawBalls( DrawType );
	void drawRibbons( DrawType );
	// doing your own drawing? use these. these use geometry generated in prepareToDraw()
	
	void setContours( const ContourVec& contours, ContourVec::Filter filter=0 ) { mContours = contours; mContourFilter=filter; }
	
	int getBallIndex( const Ball& b ) const;
	
	virtual void onBallBallCollide			( const Ball&, const Ball& );
	virtual void onBallContourCollide		( const Ball&, const Contour& );
	virtual void onBallWorldBoundaryCollide	( const Ball& );
	// these functions record collisions. if you override, then make sure you call BallWorld::
	// if you still want them returned in the collision lists.
	
	// params
	int		mNumIntegrationSteps	= 1;	
	int		mDefaultNumBalls		= 5;
	float	mBallDefaultRadius		= 8.f *  .5f ;
	float	mBallDefaultMaxRadius	= 8.f * 4.f ;
	float	mBallMaxVel				= 8.f ;
	ColorAf mBallDefaultColor		= ColorAf::hex(0xC62D41);
	float   mBallContourImpactNormalVelImpulse = .0f; // for added excitement. if zero, then max vel matters less.
	float	mBallContourCoeffOfRestitution = 1.f; // [0,1] [elastic,inelastic]
	float	mBallContourFrictionlessCoeff = 1.f;
	
	bool	mRibbonEnabled   = false;
	int		mRibbonMaxLength = 32;
	int		mRibbonSampleRate= 1;
	float	mRibbonRadiusScale = .9f;
	float	mRibbonRadiusExp   = .5f;
	float	mRibbonAlphaScale  = .5f;
	float	mRibbonAlphaExp    = .5f;
	
	// ribbons
	int  getRibbonMaxLength() const { return mRibbonEnabled ? mRibbonMaxLength : 0; }
	void accumulateBallHistory();
	void updateBallsWithRibbonParams();
	
private:	
	// storing collisions
	BallBallCollisionVec	mBallBallCollisions;
	BallContourCollisionVec	mBallContourCollisions;
	BallWorldCollisionVec	mBallWorldCollisions;
	
	// simulation
	int mLastNumIntegrationSteps=1;
	
	void scaleBallVelsForIntegrationSteps( int oldSteps, int newSteps );
	void updatePhysics();

	vec2 unlapEdge( vec2 p, float r, const Contour& poly, const Ball* b=0 );
	vec2 unlapHoles( vec2 p, float r, ContourKind kind, const Ball* b=0 );
	
	//
//	vec2 resolveCollisionWithBalls		( vec2 p, float r, Ball* ignore=0, float correctionFraction=1.f ) const ;
		// simple pushes p out of overlapping balls.
		// but might take multiple iterations to respond to all of them
		// fraction is (0,1], how much of the collision correction to do.
		// [deprecated]
	
	void resolveBallContourCollisions();
	void resolveBallCollisions() ;

	ContourVector		mContours;
	ContourVector::Filter mContourFilter;
	vector<Ball>		mBalls ;

	// drawing
	void drawImmediate( bool lowPoly ) const;
	TriMeshRef getTriMeshForBalls() const;
	TriMeshRef getTriMeshForRibbons() const;
	TriMeshRef mBallMesh; // Make into a Batch for even more performance (that we don't need)
	TriMeshRef mRibbonMesh;
	
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
