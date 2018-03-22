//
//  RaceWorld.hpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 7/26/17.
//
//

#ifndef RaceWorld_hpp
#define RaceWorld_hpp

#include "BallWorld.h"
#include "GamepadManager.h"

class RaceWorld : public BallWorld
{
public:

	RaceWorld();
	~RaceWorld();
	
	string getSystemName() const override { return "RaceWorld"; }
	void setParams( XmlTree ) override;

	void gameWillLoad() override;
	void worldBoundsPolyDidChange() override;
	void update() override;
	void draw( DrawType ) override;
	void gamepadEvent( const GamepadManager::Event& ) override;

protected:

private:
	FileWatch mFileWatch;
	
	struct PlayerColor
	{
		ColorA	mShip;
		ColorA	mShipRibbon;
		ColorA	mShot;
		ColorA	mShotRibbon;
	};
	vector<PlayerColor> mPlayerColors;
	vector<int>			mPlayerColorsUsed; // how many players using this color scheme?
	int  assignNewPlayerColor();
	void freePlayerColor(int);
	
	class Tuning
	{
	public:
		bool	mShipDrawDebug				= false;
		float	mAxisDeadZone				= .2f;
		
		float	mGoalBallRadius				= 1.5f;
		ColorA  mGoalBallColor;
		int		mGoalBallSpawnWaitTicks		= 60 * 3;
		float	mGoalBallSpawnMaxVel		= .1f;
		
		float	mShotRadius					= .5f;
		float	mShotVel					= 5.f;
		float	mShotDistance				= 1.f;
		ColorA	mShotColor, mRibbonColor;
		
		int		mMultigoalOdds				= 8; 
		int		mMultigoalMax				= 4; 
		
		float	mPlayerRadius				= 2.f;
		float	mPlayerTurnSpeedScale		= .08f;
		float	mPlayerAccelSpeedScale		= .05f;
		float	mPlayerFriction				= .01f;
		int		mPlayerFireIntervalTicks	= 20;
		int		mPlayerRespawnWaitTicks		= 30;
		float	mPlayerDieSpawnGoalToScoreFrac = .5f;
		float	mPlayerScoreNotchRadius		= .1f;
		
		float	mPlayerCollideFrictionCoeff	= .1f;
		
		float	mPfxCollideDustRadius		= .5f;
		ColorA	mPfxCollideDustColor;
		float	mPfxFadeStep				= .1f;
		
		class Controls
		{
		public:
			vector<unsigned int> mFire  = {4,5}; // DS4 L,R
			vector<unsigned int> mAccel = {0,1,2}; // DS4 Square, X, O
		};
		Controls mControls;
	}
	mTuning;
	
	class Player
	{
	public:
		GamepadManager::DeviceId mGamepad=0; // for convenience
		
		vec2	mFacing=vec2(0,1);
		int		mScore=0;
		int		mBallIndex=-1;
		
		int		mFireWait=0;
		int		mSpawnWait=0;
		
		int		mColorScheme=0;
		float	mTurn=0.f;
		
		// graphics
//		ci::gl::TextureRef mImage;
	};

	class BallData : public Ball::UserData
	{
	public:
	
		~BallData() override {}
		
		enum class Type
		{
			Goal,
			Player,
			Shot,
			Unknown
		};
		
		Type						mType = Type::Unknown;
		GamepadManager::DeviceId	mGamepad=0;
		
		bool mFadeOut = false;
	};
	
	static BallData* getBallData( const Ball& b ) {
		return std::dynamic_pointer_cast<BallData>(b.mUserData).get();
	}
		
	typedef map<GamepadManager::DeviceId,Player> tDeviceToPlayerMap;
	tDeviceToPlayerMap mPlayers;

	Player* getPlayerByBallIndex( int );
	Player* getPlayerByGamepad  ( GamepadManager::DeviceId );
	
	void tickPlayer( Player& );
	void drawPlayer( const Player& ) const;
	
	void setupPlayer ( Gamepad_device* ); // idempotent
	void removePlayer( Gamepad_device* );
	
	void remapBallIndices();
	
	void tickGoalSpawn(); // maybe makes N 
	void spawnGoal( vec2 ); // makes 1
	void handleCollisions();
	void makeBullet( Player& );
	
	void tickPfx();
	void makePfx( vec2 loc, float r, ColorA c, vec2 vel, bool ribbon );
	
	//
	ci::gl::TextureRef	mShip;
	float				mShipScale=1.f;
	
	int					mGoalCount=0;
	int					mGoalBallSpawnWaitTicks=-1;
	
	
	// synthesis
	cipd::PureDataNodeRef	mPd;	// synth engine
	cipd::PatchRef			mPatch;			// pong patch
	
	void setupSynthesis() override;
	void updateSynthesis() override;

	// debug helpers
	void nanScan();
	void dumpCore();
};

#endif /* RaceWorld_hpp */
