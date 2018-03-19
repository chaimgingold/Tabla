//
//  RaceWorld.cpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 7/26/16.
//
//

#include "RaceWorld.h"
#include "geom.h"
#include "cinder/Rand.h"
#include "cinder/audio/Context.h"
#include "cinder/audio/Source.h"
#include "TablaApp.h"

static GameCartridgeSimple sCartridge("RaceWorld", [](){
	return std::make_shared<RaceWorld>();
});

RaceWorld::RaceWorld()
{
	randSeed( clock() );
	
	setupSynthesis();

	// default goal
	mTuning.mGoalBall.mUserType = BallGoal;
	mTuning.mGoalBall.mColor  = Color::hex(0xF5BF30);
	mTuning.mGoalBall.mRadius = 1.5f;
	mTuning.mGoalBall.mRibbonColor = mTuning.mGoalBall.mColor;
	mTuning.mGoalBall.mCollideWithContours = true; 
	mTuning.mGoalBall.setVel( vec2(0,0) );
	mTuning.mGoalBall.mAccel = vec2(0,0);
		
	// load ship
	{
		fs::path path = TablaApp::get()->hotloadableAssetPath( fs::path(getSystemName()) / "ship.png" );

		mFileWatch.load( path, [this]( fs::path path )
		{
			// load the texture
			gl::Texture::Format format;
			format.loadTopDown(false);
			format.mipmap(true);
			mShip = gl::Texture2d::create( loadImage(path), format );
			
			mShipScale = 1.f;
			if (mShip) {
				mShipScale = 2.f / max( mShip->getWidth(), mShip->getHeight() );
			}
		});
	}
}

void RaceWorld::gameWillLoad()
{
	// most important thing is to prevent BallWorld from doing its default thing. (makes balls)
}

void RaceWorld::worldBoundsPolyDidChange()
{
	// most important thing is to prevent BallWorld from doing its default thing. (makes balls)
}

void RaceWorld::gamepadEvent( const GamepadManager::Event& event )
{
	switch ( event.mType )
	{
		case GamepadManager::EventType::ButtonDown:
		 
			setupGamepad(event.mDevice);
			cout << "down " << event.mId << endl;
			
			break;

		case GamepadManager::EventType::ButtonUp:
			setupGamepad(event.mDevice);
			cout << "up "  << event.mId << endl;
			break;
			
		case GamepadManager::EventType::AxisMoved:
			break;

		case GamepadManager::EventType::DeviceAttached:
			cout << "attached "  << event.mDevice << endl;
			break;
			
		case GamepadManager::EventType::DeviceRemoved:
			cout << "removed "  << event.mDevice << endl;
			removePlayer(event.mDevice);
			break;
	}
}

void RaceWorld::setupGamepad( Gamepad_device* gamepad )
{
	if ( gamepad && mPlayers.find(gamepad->deviceID) == mPlayers.end() )
	{
		Player p;
	
		Ball &ball = newRandomBall( getRandomPointInWorldBoundsPoly() );
		ball.mUserType = BallPlayer;
		ball.mUserID   = mNextPlayerId;
		ball.mCollideWithContours = true;
		ball.setVel( vec2(0,0) );
		ball.mAccel = vec2(0,0);
		
		p.mBallIndex = getBalls().size()-1; // assume new one is at the end :)
		p.mBallUserId = mNextPlayerId;
		p.mGamepad = gamepad->deviceID;
		
		mPlayers[gamepad->deviceID] = p;
		
		mNextPlayerId++;
	}
}

void RaceWorld::removePlayer( Gamepad_device* gamepad )
{
	auto it = mPlayers.find(gamepad->deviceID);
	
	if ( it != mPlayers.end() )
	{
		if (it->second.mBallIndex != -1)
		{
			eraseBall(it->second.mBallIndex);
			remapBallIndices();
		}
		
		mPlayers.erase(it);
	}
}

void RaceWorld::remapBallIndices()
{
	auto u = getBallsByUserID();
	
	for( auto p : mPlayers )
	{
		auto i = u.find(p.second.mBallUserId);
	
		p.second.mBallIndex = (i==u.end()) ? -1 : i->second;
	}
}

void RaceWorld::tickPlayer( Player& p )
{
	// get ball
	// (and ensure ball still here)
	if (p.mBallIndex == -1) return;
	
	Ball &ball = getBalls()[p.mBallIndex];
	
	// get gamepad
	const GamepadManager::Device* gamepad = getGamepadManager().getDeviceById(p.mGamepad);
	if (gamepad)
	{
		// accel
		if (gamepad->buttonStates[0])
		{
			ball.mAccel += p.mFacing * .05f;
			// ideally we let the engine rev up and down so this happens smoothly.. (and feels a bit sloppy)
		}
//		else
		{
			// "friction"
			float v = length( ball.getVel() );
			if ( v > 0.f )
			{
				ball.mAccel += -min(v,.01f) * normalize(ball.getVel());
			}
			// TODO: do this in a more graceful way
		}
		
		// rotate
		p.mFacing = glm::rotate( p.mFacing, gamepad->axisStates[0] * .08f );
	}
}

void RaceWorld::drawPlayer( const Player& p ) const
{
	if (p.mBallIndex==-1) return;	
	const Ball &ball = getBalls()[p.mBallIndex];
	
	if ( mTuning.mShipDrawDebug || !mShip )
	{
		gl::color(ball.mColor);
		gl::drawLine( ball.mLoc, ball.mLoc + ball.mRadius * 2.f * p.mFacing );
		
		gl::pushModelMatrix();
		gl::translate( ball.mLoc );
		gl::multModelMatrix( mat4( mat2( -perp(p.mFacing), p.mFacing ) ) );
		gl::scale( vec2(ball.mRadius) );

		gl::drawSolidRect( Rectf(-.5,-1,.5,1.5) );

		gl::popModelMatrix();
	}

	if (mShip)
	{
		gl::pushModelMatrix();

		gl::translate( ball.mLoc );
		gl::multModelMatrix( mat4( mat2( -perp(p.mFacing), -p.mFacing ) ) );
		gl::scale( vec2(ball.mRadius) * mShipScale );

		gl::color(1,1,1,1);
		gl::draw( mShip, -mShip->getSize() / 2 );

		gl::popModelMatrix();
	}
}

void RaceWorld::update()
{
	// players
	for( auto &p : mPlayers )
	{
		tickPlayer(p.second);
	}

	// goal
	tickGoalSpawn();

	// collisions
	handleCollisions();

	//
	BallWorld::update(); // also does its sound, which may be an issue
}

void RaceWorld::tickGoalSpawn()
{
	mGoalCount=0;
	
	auto t = getBallsByUserType();
	auto i = t.find(BallGoal); 
	if ( i != t.end() ) mGoalCount = i->second.size();
	
	if ( mGoalCount==0 )
	{
		if ( mGoalBillSpawnWaitTicks == -1 ) {
			 mGoalBillSpawnWaitTicks = mTuning.mGoalBillSpawnWaitTicks;
		}
		
		if ( mGoalBillSpawnWaitTicks > 0 ) {
			 mGoalBillSpawnWaitTicks--;
		}
		
		if ( mGoalBillSpawnWaitTicks==0 )
		{
			// new goal!
			int n = 1;
			
			for( int i=0; i<mTuning.mMultigoalMax; ++i )
			{
				if (randInt()%mTuning.mMultigoalOdds==0) {
					n++;	
				}
			}
			
			for ( int i=0; i<n; ++i )
			{
				Ball& b = newRandomBall( getRandomPointInWorldBoundsPoly() );
				
				b = mTuning.mGoalBall;
				b.mHistory.set_capacity( getRibbonMaxLength() );
				
				b.setLoc( getRandomPointInWorldBoundsPoly() );				
				b.setVel( randVec2() * randFloat() * .1f );
				
				mGoalCount++;
			}
	
			mGoalBillSpawnWaitTicks = -1;
		}
	}
}

void RaceWorld::handleCollisions()
{
	auto bbc = getBallBallCollisions();
	auto balls = getBalls();
	
	vector<int> removeGoals;
	
	for( BallBallCollision c : bbc )
	{
		// make sure player is in slot 0
		if ( balls[c.mBallIndex[1]].mUserType == BallPlayer )
		{
			swap( c.mBallIndex[0], c.mBallIndex[1] );
		}
		
		// 0 isa player 
		if ( balls[c.mBallIndex[0]].mUserType == BallPlayer )
		{
			if ( balls[c.mBallIndex[1]].mUserType == BallGoal )
			{
				removeGoals.push_back(c.mBallIndex[1]);
				
				// score it
				Player* p = getPlayerByBallIndex( c.mBallIndex[0] );
				if (p) p->mScore++;
			}
		}
	}
	
	// remove stuff
	for ( auto g : removeGoals ) {
		eraseBall(g);
	}
	
	if ( !removeGoals.empty() ) {
		remapBallIndices();
	}
}

RaceWorld::Player*
RaceWorld::getPlayerByBallIndex( int bi )
{
	for ( auto &i : mPlayers )
	{
		if ( i.second.mBallIndex==bi ) return &(i.second);
	}
	
	return 0;
}

void RaceWorld::draw( DrawType drawType )
{
	mFileWatch.update();
	
	//
	// Draw world
	//
	BallWorld::draw(drawType);
	
	for( auto &p : mPlayers )
	{
		drawPlayer(p.second);
	}
}

// Synthesis
void RaceWorld::setupSynthesis()
{
	mPd = TablaApp::get()->getPd();

	// Load pong synthesis patch
	auto app = TablaApp::get();
	std::vector<fs::path> paths =
	{
		app->hotloadableAssetPath("synths/RaceWorld/pong-world.pd"),
		app->hotloadableAssetPath("synths/RaceWorld/pong-voice.pd"),
		app->hotloadableAssetPath("synths/RaceWorld/score-voice.pd")
	};

	// Register file-watchers for all the major pd patch components
	mFileWatch.load( paths, [this,app]()
					{
						// Reload the root patch
						auto rootPatch = app->hotloadableAssetPath("synths/RaceWorld/pong-world.pd");
						mPd->closePatch(mPatch);
						mPatch = mPd->loadPatch( DataSourcePath::create(rootPatch) ).get();
					});
}

RaceWorld::~RaceWorld() {
	// Close pong synthesis patch
	mPd->closePatch(mPatch);
}
