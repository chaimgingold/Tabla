//
//  PongWorld.hpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 9/20/16.
//
//

#ifndef PongWorld_hpp
#define PongWorld_hpp

#include "BallWorld.h"
#include "PureDataNode.h"

class PongWorld : public BallWorld
{
public:

	PongWorld();
	~PongWorld();
	
	string getSystemName() const override { return "PongWorld"; }

	void gameWillLoad() override;
	void update() override;
	void draw( DrawType ) override;
	
	void worldBoundsPolyDidChange() override;

protected:
	virtual void onBallBallCollide   ( const Ball&, const Ball& ) override;
	virtual void onBallContourCollide( const Ball&, const Contour& ) override;
	virtual void onBallWorldBoundaryCollide	( const Ball& ) override;

private:
	
	void drawScore( int player, vec2 dotStart, vec2 dotStep, float dotRadius, int score, int maxScore ) const ;
	
	// player info
	Color mPlayerColor[2];
	int   mPlayerScore[2];
	
	// field layout
	void computeFieldLayout( int orientation=-1 ); // -1 picks it for you; can run recursively
	
	int  mFieldPlayer0LeftCornerIndex=0;
		// it could be 0 or 1 (higher numbers would be equivalent)
		// this orients our four sided field to the world polygon
		// in effect, it picks the orientation of play

	vec2 fieldCorner( int i ) const;
	
	vec2 mPlayerGoal[2];// g
	vec2 mPlayerSide[2][2]; // (0,3), (1,2)
	vec2 mPlayerVec[2]; // p0, p1
	vec2 mCenterVec;	// b -> a
	vec2 mCenterLine[2];// a, b
	vec2 mCenter;		// c
	
		/*        ^ centerVec
		      0---a---1
		      |   |   |
		<- p0 |)  c  (| p1 ->
		      |   |   |
		      3---b---2
			  
			  Corners fieldCorner(0,1,2,3), relative to mFieldPlayer0LeftCornerIndex
			  
			  All this assumes world bounds has four corners.
		*/

	float mFieldCenterCircleRadius;
	
	float mScoreDotRadius;
	vec2 mScoreDotStart[2];
	vec2 mScoreDotStep[2];
	
	// game rules
	int mMaxScore;
	
	void didScore( int player );
	void serve();
	
	enum class GameState
	{
		Attract,// waiting for play to start
		Serve,	// serve in progress
		Play,	// ball is in play
		Score,	// someone just scored
		Over	// someone won
	};
	string getStateName( GameState ) const;
	// might be nice to do this will strings entirely, and then have timeouts etc stored in a tuning file.
	// then it can be generalized to our other games easily, too.
	
	GameState mState;
	float mStateEnterTime;
	int mPlayerJustScored;
	
	float getSecsInState() const; // how long we been in this state?
	
	void goToState( GameState );
	void stateDidChange( GameState old, GameState newState );
	
	// feedback
	void strobeBalls();
	
	// synthesis
	cipd::PureDataNodeRef	mPd;	// synth engine
	cipd::PatchRef			mPatch;			// pong patch
	
	void setupSynthesis();
};

class PongWorldCartridge : public GameCartridge
{
public:
	virtual string getSystemName() const override { return "PongWorld"; }

	virtual std::shared_ptr<GameWorld> load() const override
	{
		return std::make_shared<PongWorld>();
	}
};

#endif /* PongWorld_hpp */
