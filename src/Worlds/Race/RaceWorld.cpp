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

void FX( const char* comment=0, bool print=true )
{
	if (comment&&print) cout << comment << endl;
	// placeholder for particle, sound fx.
}

RaceWorld::RaceWorld()
{
	randSeed( clock() );
	
	setupSynthesis();

	// default goal
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


void RaceWorld::setParams( XmlTree xml )
{
	BallWorld::setParams(xml);
	
	if (xml.hasChild("Tuning"))
	{
		XmlTree t = xml.getChild("Tuning");
		
		getXml(t,"ShipDrawDebug",			mTuning.mShipDrawDebug);
		getXml(t,"AxisDeadZone",			mTuning.mAxisDeadZone);
		
		getXml(t,"GoalBall/SpawnWaitTicks",	mTuning.mGoalBallSpawnWaitTicks);
		getXml(t,"GoalBall/Color",			mTuning.mGoalBall.mColor);
		getXml(t,"GoalBall/Radius",			mTuning.mGoalBall.mRadius);
		getXml(t,"GoalBall/SpawnMaxVel",	mTuning.mGoalBallSpawnMaxVel);

		getXml(t,"Shot/Color",				mTuning.mShotColor);
		getXml(t,"Shot/RibbonColor",		mTuning.mRibbonColor);
		getXml(t,"Shot/Radius",				mTuning.mShotRadius);
		getXml(t,"Shot/Vel",				mTuning.mShotVel);
		getXml(t,"Shot/Distance",			mTuning.mShotDistance);
		
		getXml(t,"MultigoalOdds", 			mTuning.mMultigoalOdds );
		getXml(t,"MultigoalMax",			mTuning.mMultigoalMax );
		
		getXml(t,"PlayerTurnSpeedScale",	mTuning.mPlayerTurnSpeedScale );
		getXml(t,"PlayerAccelSpeedScale",	mTuning.mPlayerAccelSpeedScale );
		getXml(t,"PlayerFriction",			mTuning.mPlayerFriction );
		getXml(t,"PlayerCollideFrictionCoeff", mTuning.mPlayerCollideFrictionCoeff );
		getXml(t,"PlayerFireIntervalTicks",	mTuning.mPlayerFireIntervalTicks );
		
		mTuning.mGoalBall.mRibbonColor = mTuning.mGoalBall.mColor;
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
		 
			setupPlayer(event.mDevice);
			cout << "down " << event.mId << endl;
			FX("button down");
			break;

		case GamepadManager::EventType::ButtonUp:
			setupPlayer(event.mDevice);
			cout << "up "  << event.mId << endl;
			FX("button up");
			break;
			
		case GamepadManager::EventType::AxisMoved:
			break;

		case GamepadManager::EventType::DeviceAttached:
			cout << "attached "  << event.mDevice << endl;
			FX("gamepad attach");
			break;
			
		case GamepadManager::EventType::DeviceRemoved:
			cout << "removed "  << event.mDevice << endl;
			removePlayer(event.mDevice);
			FX("gamepad detach");
			break;
	}
}

void RaceWorld::setupPlayer( Gamepad_device* gamepad )
{
	if ( gamepad )
	{
		Player* player = getPlayerByGamepad(gamepad->deviceID);
		
		// make player	
		if ( !player )
		{
			Player p;
			p.mBallIndex = -1;
			p.mGamepad   = gamepad->deviceID;
			p.mFacing    = randVec2();
			mPlayers[gamepad->deviceID] = p;
			
			getPlayerByGamepad(gamepad->deviceID);
		}
		
		// make ship
		if ( player && player->mBallIndex == -1 )
		{
			FX("player spawn");

			Ball &ball = newRandomBall( getRandomPointInWorldBoundsPoly() );
			ball.mCollideWithContours = true;
			ball.setVel( vec2(0,0) );
			ball.mAccel = vec2(0,0);

			auto ballData = make_shared<BallData>();
			ballData->mGamepad = player->mGamepad;
			ballData->mType = BallData::Type::Player;
			ball.mUserData = ballData;
			
			player->mBallIndex = getBalls().size()-1; // last ball we added is it
		}
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
		}
		
		mPlayers.erase(it);
	}

	remapBallIndices();
}

void RaceWorld::remapBallIndices()
{
	// clear
	for( auto p : mPlayers )
	{
		p.second.mBallIndex = -1;
	}
	
	// link
	const vector<Ball>& balls = getBalls();
	
	for( const Ball &b : balls )
	{
		auto bd = getBallData(b);
		
		if (bd && bd->mType == BallData::Type::Player)
		{
			auto p = getPlayerByGamepad(bd->mGamepad);
			if (p) p->mBallIndex = getBallIndex(b);
		}
	}
}

void RaceWorld::makeBullet( Player& p )
{
	FX("shoot");
	if (p.mBallIndex == -1) return;
	
	const Ball& pb = getBalls()[p.mBallIndex];
	
	Ball& b = newRandomBall( getRandomPointInWorldBoundsPoly() );
	// TODO: stop using newRandomBall. it's makin us crazy with stoopid bugs. 
	
	b.mColor		= mTuning.mShotColor;
	b.mRibbonColor	= mTuning.mRibbonColor;
	b.mRadius		= mTuning.mShotRadius;
	b.mCollideWithContours = true;
	
	vec2 v = p.mFacing;
	
	b.setLoc( pb.mLoc + v * (pb.mRadius + b.mRadius + mTuning.mShotDistance) );
	b.setVel( vec2(0.f) );
	b.mAccel = v * mTuning.mShotVel;

	auto ballData = make_shared<BallData>();
	ballData->mType		= BallData::Type::Shot;
	ballData->mGamepad	= p.mGamepad;
	b.mUserData = ballData;	
}

void RaceWorld::tickPlayer( Player& p )
{
	auto button = []( const GamepadManager::Device* d, const vector<unsigned int>& btn ) -> bool
	{
		if (d) {
			for( auto b : btn ) {
				if ( b < d->numButtons && d->buttonStates[b] ) {
					return true;
				}
			}
		}
		
		return false;
	};
	
	// get ball
	// (and ensure ball still here)
	if (p.mBallIndex == -1) return;
	
	Ball &ball = getBalls()[p.mBallIndex];
	
	// get gamepad
	const GamepadManager::Device* gamepad = getGamepadManager().getDeviceById(p.mGamepad);

	// accel
	if ( button(gamepad,mTuning.mControls.mAccel) )
	{
		FX("accel",false);
		ball.mAccel += p.mFacing * mTuning.mPlayerAccelSpeedScale;
		// ideally we let the engine rev up and down so this happens smoothly.. (and feels a bit sloppy)
	}

	// shoot
	if ( p.mFireWait > 0 ) p.mFireWait--; // cool off
	
	if ( button(gamepad,mTuning.mControls.mFire) )
	{
		if ( p.mFireWait <= 0 )
		{
			// fire!
			makeBullet(p);
			p.mFireWait = mTuning.mPlayerFireIntervalTicks;
		}
		else FX("can't fire; cool off",false);
	}
	
	// "friction"
	{
		float v = length( ball.getVel() );
		
		float f = mTuning.mPlayerFriction;
		f += length(ball.mSquash) * mTuning.mPlayerCollideFrictionCoeff;
		
		if ( v > 0.f )
		{
			ball.mAccel += -min(v,f) * normalize(ball.getVel());
		}
		// TODO: do this in a more graceful way
	}
	
	// rotate
	if (gamepad) {
		if ( fabs(gamepad->axisStates[0]) > mTuning.mAxisDeadZone ) {
			p.mFacing = glm::rotate( p.mFacing, gamepad->axisStates[0] * mTuning.mPlayerTurnSpeedScale );
			FX("turn",false);
		}
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
	// players (controls)
	for( auto &p : mPlayers )
	{
		tickPlayer(p.second);
	}

	// physics
	BallWorld::update(); // also does its sound, which may be an issue

	// handle collisions (right after physics!)
	handleCollisions();

	// goal spawn
	tickGoalSpawn();
}

void RaceWorld::tickGoalSpawn()
{
	mGoalCount=0;
	
	for ( auto b : getBalls() )
	{
		BallData* d = getBallData(b);
		if ( d && d->mType == BallData::Type::Goal ) {
			mGoalCount++;
		}
	}
	
	if ( mGoalCount==0 )
	{
		if ( mGoalBallSpawnWaitTicks == -1 ) {
			 mGoalBallSpawnWaitTicks = mTuning.mGoalBallSpawnWaitTicks;
		}
		
		if ( mGoalBallSpawnWaitTicks > 0 ) {
			 mGoalBallSpawnWaitTicks--;
		}
		
		if ( mGoalBallSpawnWaitTicks==0 )
		{
			FX("goal spawn");
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
				b.setVel( randVec2() * randFloat() * mTuning.mGoalBallSpawnMaxVel );

				auto ballData = make_shared<BallData>();
				ballData->mType = BallData::Type::Goal;
				b.mUserData = ballData;
				
				mGoalCount++;
			}
	
			mGoalBallSpawnWaitTicks = -1;
		}
	}
}

void RaceWorld::handleCollisions()
{
	const vector<Ball>& balls = getBalls();
	set<size_t> removeBall;	
	
	// ball-world and ball-contour collisions
	{
		auto bwc = getBallWorldCollisions();
		auto bcc = getBallContourCollisions();
		
		auto test = [&balls,&removeBall]( int i )
		{
			auto bd = getBallData( balls[i] );
			
			// shot hit wall/world
			if (bd && bd->mType == BallData::Type::Shot )
			{
				// DINK!
				removeBall.insert(i);
				FX("shot hit world/wall");
			}			
		};
		
		for( auto c : bcc ) test(c.mBallIndex);
		for( auto c : bwc ) test(c.mBallIndex);
	}
	
	// ball-ball collisions
	{
		auto bbc = getBallBallCollisions();
		
		for( BallBallCollision c : bbc )
		{
			BallData *d[2] = {
				getBallData( getBalls()[ c.mBallIndex[0] ] ),
				getBallData( getBalls()[ c.mBallIndex[1] ] )
			};
			if ( !d[0] || !d[1] ) continue; // hit a UFO (ie no ball data)
			
			// make sure player is in slot 0
			if ( d[1]->mType == BallData::Type::Player )
			{
				swap( c.mBallIndex[0], c.mBallIndex[1] );
				swap( d[0], d[1] );
			}
			
			// Player hits X
			// 0 isa player 
			if ( d[0]->mType == BallData::Type::Player )
			{
				switch( d[1]->mType )
				{
					// goal
					case BallData::Type::Goal:
					{
						removeBall.insert(c.mBallIndex[1]);
						
						// score it
						Player* p = getPlayerByGamepad( d[0]->mGamepad );
						if (p) p->mScore++;
						FX("get goal");
					}
					break;
					
					// bullet
					case BallData::Type::Shot:
					{
						// no self-kill
						if ( d[0]->mGamepad != d[1]->mGamepad )
						{
							// kaboom
							removeBall.insert(c.mBallIndex[1]); // shot
							removeBall.insert(c.mBallIndex[0]); // player
							FX("player die");
							
							// update player
							Player* p = getPlayerByBallIndex( c.mBallIndex[0] );
							if (p)
							{
								p->mBallIndex = -1;
							}
							
							// TODO: spawn coins for score * multiplier
						}
					}
					break;

					// player
					case BallData::Type::Player:
					{
						// sound???
						FX("player hit player");
					}
					break;
									
					default:break;
				}
			}
		}
	}
		
	// remove stuff
	eraseBalls(removeBall);
	
	if ( !removeBall.empty() ) {
		remapBallIndices();
	}
}

RaceWorld::Player*
RaceWorld::getPlayerByBallIndex( int bi )
{
	auto bd = getBallData( getBalls()[bi] );
	
	if (bd) return getPlayerByGamepad( bd->mGamepad );
	else return 0;
}

RaceWorld::Player*
RaceWorld::getPlayerByGamepad( GamepadManager::DeviceId id )
{
	auto i = mPlayers.find(id);
	if ( i == mPlayers.end() ) return 0;
	else return &(i->second);
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
