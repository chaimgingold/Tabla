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
#include "PureDataNode.h"
#include "MIDIInput.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class Ball {
	
public:
	Ball() {
		mCollideWithContours	= 1;
		mPausePhysics			= 0;
		mCapVelocity			= 1;
	}
	
	vec2 mLoc ;
	vec2 mLastLoc ;
	vec2 mAccel ;
	
	float mRadius ;
	ColorAf mColor ;
	ColorAf mRibbonColor=ColorAf(0,0,0,0) ;
	
	void setLoc( vec2 l ) { vec2 v=getVel(); mLoc=l; setVel(v); }
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
	
	// flags
	unsigned int mCollideWithContours : 1; //=1 by default; if 0, collide with inverse contours
	unsigned int mPausePhysics		  : 1; //=0 by default
	unsigned int mCapVelocity		  : 1; //=1 by default
	uint32_t	 mCollisionMask				= 1; // what to collide against?
	
	// history (for ribbons)
	boost::circular_buffer<vec2> mHistory;
	
	// User data
	// (BallWorld doesn't use it)
	class UserData {
  	  public:
		virtual ~UserData() {}
	};	
	std::shared_ptr<UserData> mUserData;
	
private:
	float	mMass = 1.f ; // let's start by doing the right thing.

};


class BallWorld : public GameWorld
{
public:
	
	BallWorld();
	~BallWorld();
	
	string getSystemName() const override { return "BallWorld"; }
	
	void setParams( XmlTree ) override;
	void updateVision( const Vision::Output& c, Pipeline& ) override { setContours(c.mContours); }
	
	void gameWillLoad() override; // make some balls by default
	void update() override;
	void prepareToDraw() override;
	void draw( DrawType ) override;
	
	Ball& newRandomBall( vec2 loc ); // returns it, too, if you want to modify it.
	void clearBalls() { mBalls.clear(); }
	void eraseBall ( int index );
	void eraseBalls( std::set<size_t> indices );
	
	float getBallDefaultRadius() const { return mBallDefaultRadius ; }
	int   getTargetBallCount() const;
	virtual void worldBoundsPolyDidChange() override;
	
	vec2 resolveCollisionWithContours		( vec2 p, float r, const Ball* b=0 );
	vec2 resolveCollisionWithInverseContours( vec2 p, float r, const Ball* b=0 );
		// returns pinned version of point
		// public so we can show it with the mouse...
		// optional Ball* so we can record collisions (onBall*Collide())
		// not const because if b != 0 then we will note collisions and mutate state
	
	void mouseClick( vec2 p ) override { newRandomBall(p) ; }
	void keyDown( KeyEvent ) override;
	void drawMouseDebugInfo( vec2 ) override;
	
	void setNumBalls( int );
	
	vector<Ball>& getBalls() { return mBalls; }
	const vector<Ball>& getBalls() const { return mBalls; }
	// findBallWithUserData
	
	vec2 getDenoisedBallVel( const Ball& ) const; // tries to use mHistory if it's there, otherwise just mVel
	
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
		BallContourCollision( int a, int b, vec2 pt )
			: mBallIndex(a)
			, mContourIndex(b)
			, mPt(pt) {}

		BallContourCollision() : BallContourCollision(-1,-1,vec2(0.f)){} 

		int mBallIndex;
		int mContourIndex;
		vec2 mPt;
	};
	typedef vector<BallContourCollision> BallContourCollisionVec;

	struct BallWorldCollision {
	public:
		BallWorldCollision( int a=-1, vec2 pt=vec2(0.f) ) { mBallIndex=a; mPt=pt; }
		int mBallIndex;
		vec2 mPt;
	};
	typedef vector<BallWorldCollision> BallWorldCollisionVec;

	const BallBallCollisionVec&		getBallBallCollisions() const { return mBallBallCollisions; }
	const BallContourCollisionVec&	getBallContourCollisions() const { return mBallContourCollisions; }
	const BallWorldCollisionVec&	getBallWorldCollisions() const { return mBallWorldCollisions; }
	
	int getNumIntegrationSteps() const { return mNumIntegrationSteps; }

	void drawBalls( DrawType, gl::GlslProgRef=0 ) const;
	void drawRibbons( DrawType ) const;
	// doing your own drawing? use these. these use geometry generated in prepareToDraw()

	// synthesis
	cipd::PureDataNodeRef mPd;
	cipd::PatchRef	      mPatch;
	virtual void setupSynthesis();
	virtual void updateSynthesis();
		
protected:

	void setContours( const ContourVec& contours, ContourVec::Filter filter=0 ) { mContours = contours; mContourFilter=filter; }
	
	int getBallIndex( const Ball& b ) const;
	
	virtual void onBallBallCollide			( const Ball&, const Ball& );
	virtual void onBallContourCollide		( const Ball&, const Contour&, vec2 );
	virtual void onBallWorldBoundaryCollide	( const Ball&, vec2 );
	// these functions record collisions. if you override, then make sure you call BallWorld::
	// if you still want them returned in the collision lists.
	
	void maybeUpdateBallPopulationWithDesiredDensity(); // if mBallDensity<0 then ignored
	
	// params
	int		mNumIntegrationSteps	= 1;	
	float	mBallDensity			= -1.f; // negative for disable auto ball populate, positive for how much
	float	mBallDefaultRadius		= 8.f *  .5f ;
	float	mBallDefaultMaxRadius	= 8.f * 4.f ;
	float	mBallMaxVel				= 8.f ;
	ColorAf mBallDefaultColor		= ColorAf::hex(0xC62D41);
	ColorAf mBallDefaultRibbonColor	= ColorAf(1,1,1,1);
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

	mat4 getBallTransform( const Ball& b, mat4* fixNormalMatrix=0 ) const;
	
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

	// wacky midi input experiment
	MIDIInput mMidiInput;

} ;

#endif /* Balls_hpp */
