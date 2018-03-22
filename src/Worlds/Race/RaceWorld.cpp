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

const static bool kNanScan = false;

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
		getXml(t,"GoalBall/Color",			mTuning.mGoalBallColor);
		getXml(t,"GoalBall/Radius",			mTuning.mGoalBallRadius);
		getXml(t,"GoalBall/SpawnMaxVel",	mTuning.mGoalBallSpawnMaxVel);

		getXml(t,"Shot/Radius",				mTuning.mShotRadius);
		getXml(t,"Shot/Vel",				mTuning.mShotVel);
		getXml(t,"Shot/Distance",			mTuning.mShotDistance);

		getXml(t,"MultigoalOdds", 			mTuning.mMultigoalOdds );
		getXml(t,"MultigoalMax",			mTuning.mMultigoalMax );

		getXml(t,"AttractAnimLength",		mTuning.mAttractAnimLength );
		
		getXml(t,"PlayerRadius",	mTuning.mPlayerRadius );
		getXml(t,"PlayerTurnSpeedScale",	mTuning.mPlayerTurnSpeedScale );
		getXml(t,"PlayerAccelSpeedScale",	mTuning.mPlayerAccelSpeedScale );
		getXml(t,"PlayerFriction",			mTuning.mPlayerFriction );
		getXml(t,"PlayerCollideFrictionCoeff", mTuning.mPlayerCollideFrictionCoeff );
		getXml(t,"PlayerFireIntervalTicks",	mTuning.mPlayerFireIntervalTicks );
		getXml(t,"PlayerRespawnWaitTicks",	mTuning.mPlayerRespawnWaitTicks );
		getXml(t,"PlayerDieSpawnGoalToScoreFrac", mTuning.mPlayerDieSpawnGoalToScoreFrac );
		getXml(t,"PlayerScoreNotchRadius",	mTuning.mPlayerScoreNotchRadius );

		getXml(t,"Pfx/CollideDustRadius", mTuning.mPfxCollideDustRadius );
		getXml(t,"Pfx/CollideDustColor",  mTuning.mPfxCollideDustColor  );

		getXml(t,"Pfx/FadeStep", mTuning.mPfxFadeStep );
	}
	
	mPlayerColors.clear();
	mPlayerColorsUsed.clear();
	for ( auto i = xml.begin("PlayerColors/p"); i != xml.end(); ++i )
	{
		PlayerColor p;
		getXml( *i, "Ship",		 p.mShip );
		getXml( *i, "ShipRibbon",p.mShipRibbon );
		getXml( *i, "Shot",		 p.mShot );
		getXml( *i, "ShotRibbon", p.mShotRibbon );
		mPlayerColors.push_back(p);
	}
	if (mPlayerColors.size()==0) {
		// ensure >0
		PlayerColor p;
		mPlayerColors.push_back(p);
	}
	mPlayerColorsUsed.resize(mPlayerColors.size(),0);
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

int RaceWorld::assignNewPlayerColor()
{
	assert( !mPlayerColorsUsed.empty() );

	int p=-1;

	for( int i=0; i<mPlayerColors.size(); ++i )
	{
		if ( mPlayerColorsUsed[i] == 0 ) {
			p = i;
			break;
		}
	}

	if (p==-1) {
		p = randInt() % mPlayerColors.size();
	}

	mPlayerColorsUsed[p]++;

	return p;
}

void RaceWorld::freePlayerColor(int i)
{
	mPlayerColorsUsed[i]--;
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
			p.mColorScheme = assignNewPlayerColor();
			mPlayers[gamepad->deviceID] = p;

			getPlayerByGamepad(gamepad->deviceID);
		}

		// make ship
		if ( player
		  && player->mBallIndex == -1
		  && player->mSpawnWait <= 0
		   )
		{
			FX("player spawn");
			mPd->sendBang("ship-spawn");

			Ball ball;
			PlayerColor pc = mPlayerColors[ player->mColorScheme ];

			ball.mRadius = mTuning.mPlayerRadius;
			ball.setMass( M_PI * powf(ball.mRadius,3.f) ) ;
			ball.mHistory.set_capacity(getRibbonMaxLength());

			ball.setLoc( getRandomPointInWorldBoundsPoly() );
			ball.setVel( vec2(0,0) );
			ball.mAccel = vec2(0,0);
			ball.mColor = pc.mShip;
			ball.mRibbonColor = pc.mShipRibbon;

			auto ballData = make_shared<BallData>();
			ballData->mGamepad = player->mGamepad;
			ballData->mType = BallData::Type::Player;
			ballData->mAttractAnim = 0.f;
			ball.mUserData = ballData;

			getBalls().push_back(ball);
			player->mBallIndex = getBalls().size()-1; // last ball we added is it
		}
	}
}

void RaceWorld::removePlayer( Gamepad_device* gamepad )
{
	auto it = mPlayers.find(gamepad->deviceID);

	if ( it != mPlayers.end() )
	{
		Player &p = it->second;

		if (p.mBallIndex != -1) {
			eraseBall(p.mBallIndex);
		}

		freePlayerColor(p.mColorScheme);
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
	// Trigger LASER SOUND
	mPd->sendFloat("ship-laser", p.mColorScheme );

	if (p.mBallIndex == -1) return;

	vec2 playerLoc     = getBalls()[p.mBallIndex].mLoc;
	float playerRadius = getBalls()[p.mBallIndex].mRadius;
	PlayerColor pc = mPlayerColors[ p.mColorScheme ];

	Ball b;

	b.mColor		= pc.mShot;
	b.mRibbonColor	= pc.mShotRibbon;
	b.mRadius		= mTuning.mShotRadius;
	b.setMass( M_PI * powf(b.mRadius,3.f) ) ;
	b.mHistory.set_capacity(getRibbonMaxLength());

	vec2 v = p.mFacing;

	b.setLoc( playerLoc + v * (playerRadius + b.mRadius + mTuning.mShotDistance) );
	b.setVel( vec2(0.f) );
	b.mAccel = v * mTuning.mShotVel;

	auto ballData = make_shared<BallData>();
	ballData->mType		= BallData::Type::Shot;
	ballData->mGamepad	= p.mGamepad;
	b.mUserData = ballData;
	
	getBalls().push_back(b);
}

void RaceWorld::tickPfx()
{
	set<size_t> removeBall;	
	
	for ( int i=0; i<getBalls().size(); ++i )
	{
		Ball &b = getBalls()[i];
		auto bd = getBallData(b);
		
		if (bd && bd->mFadeOut)
		{
			b.mColor.a		 -= mTuning.mPfxFadeStep;
			b.mRibbonColor.a -= mTuning.mPfxFadeStep;
			
			b.mRibbonColor.a = max( 0.f, b.mRibbonColor.a );
			
			if ( b.mColor.a <= 0.f ) {
				removeBall.insert(i);
			}
		}
	}	

	eraseBalls(removeBall);
	
	if ( !removeBall.empty() ) {
		remapBallIndices();
	}
}

void RaceWorld::makePfx( vec2 loc, float r, ColorA c, vec2 vel, bool ribbon )
{
	Ball b;
	
	b.setLoc(loc);
	b.mRadius = r;
	b.mColor = c;
	b.mRibbonColor = c;
	b.setVel(vel);
	b.mCollisionMask=0;
	
	auto ud = make_shared<BallData>();
	ud->mFadeOut = true;
	b.mUserData = ud;
	
	if (ribbon) b.mHistory.set_capacity(getRibbonMaxLength());
	
	getBalls().push_back(b);
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

	if (p.mBallIndex == -1)
	{
		// dead
		if ( p.mSpawnWait > 0 ) p.mSpawnWait--;
	}
	else
	{
		// alive
		// get gamepad
		const GamepadManager::Device* gamepad = getGamepadManager().getDeviceById(p.mGamepad);

		// accel
		if ( button(gamepad,mTuning.mControls.mAccel) )
		{
			FX("accel",false);
			getBalls()[p.mBallIndex].mAccel += p.mFacing * mTuning.mPlayerAccelSpeedScale;
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
			auto &ball = getBalls()[p.mBallIndex];

			float v = length( ball.getVel() );

			float f = mTuning.mPlayerFriction;
			f += length(getBalls()[p.mBallIndex].mSquash) * mTuning.mPlayerCollideFrictionCoeff;
			f = max(0.f,f); // DON'T ASK. workaround for a buf causing f=-inf
			
			if ( v > 0.f )
			{
				getBalls()[p.mBallIndex].mAccel += -min(v,f) * normalize(ball.getVel());
			}
			// TODO: do this in a more graceful way
		}

		// rotate
		p.mTurn = 0.f;
		if (gamepad) {
			if ( fabs(gamepad->axisStates[0]) > mTuning.mAxisDeadZone ) {
				p.mTurn = gamepad->axisStates[0] * mTuning.mPlayerTurnSpeedScale;
				p.mFacing = glm::rotate( p.mFacing, p.mTurn );
				FX("turn",false);
			}
		}
	}
}

void RaceWorld::drawPlayer( const Player& p ) const
{
	if (p.mBallIndex==-1) return;
	const Ball &ball = getBalls()[p.mBallIndex];

	gl::pushModelMatrix();

	gl::translate( ball.mLoc );
	gl::multModelMatrix( mat4( mat2( -perp(p.mFacing), -p.mFacing ) ) );


	if ( mTuning.mShipDrawDebug || !mShip )
	{
		gl::color(ball.mColor);
		gl::drawLine( ball.mLoc, ball.mLoc + ball.mRadius * 2.f * p.mFacing );

		gl::ScopedModelMatrix smm;
		gl::scale( vec2(ball.mRadius) );

		gl::drawSolidRect( Rectf(-.5,-1,.5,1.5) );
	}

	if (mShip)
	{
		gl::ScopedModelMatrix smm;
		gl::scale( vec2(ball.mRadius) * mShipScale );

		gl::color(1,1,1,1);
		gl::draw( mShip, -mShip->getSize() / 2 );
	}


	// score
	if (p.mScore>0)
	{
		gl::color( mTuning.mGoalBallColor );

		if ((1))
		{
			gl::color( mTuning.mGoalBallColor );

			float rb = ball.mRadius + .2f;
			float rs = .3f ;
			
			for( int i=0; i<p.mScore; ++i )
			{
				float dt = (sin( ((float)i / 5.f) + app::getElapsedSeconds() * 2.f ) + 1.f)/2.f;
				
				gl::drawStrokedCircle( vec2(0.f), rb + rs * (float)i + dt * .25f );
			}
		}
		else
		{	
			float step = 1.f / (float)(max(1,p.mScore-1)) ;

			float w = min( ball.mRadius * 2.f, mTuning.mPlayerScoreNotchRadius*(float)(p.mScore-1)*3.f ) ;

			vec2 e[2] =
			{
				vec2( -w/2, ball.mRadius * 1.5f ),
				vec2(  w/2, ball.mRadius * 1.5f )
			};

			for( int i=0; i<p.mScore; ++i )
			{
				vec2 c = lerp( e[0], e[1], (float)i * step );

				gl::drawSolidCircle( c, min( (float)step*2.f, mTuning.mPlayerScoreNotchRadius) );
			}
		}
	}

	gl::popModelMatrix();
}

void RaceWorld::updateSynthesis() {
	vector<Ball>& balls = getBalls();

	auto ballVels = pd::List();

	// Send no velocities when paused to keep balls silent
	//	bool shouldSynthesizeBalls = !isPaused();
	bool shouldSynthesizeBalls = true;
	if (!shouldSynthesizeBalls) {
		mPd->sendList("ship-velocities", ballVels );
		return;
	}

	for( Ball &b : balls )
	{
		BallData* d = getBallData(b);
		if ( d && d->mType == BallData::Type::Player )
		{
			float t=0.f;
			Player* p = getPlayerByGamepad(d->mGamepad);
			if (p) t = fabs(p->mTurn) * 1.f;
			ballVels.addFloat(length(getDenoisedBallVel(b))-t);
		}
	}

	mPd->sendList("ship-velocities", ballVels );
}

void RaceWorld::update()
{
	mFileWatch.update();

	nanScan();

	tickPfx();
	nanScan();
	
	// players (controls)
	for( auto &p : mPlayers )
	{
		tickPlayer(p.second);
	}
	nanScan();

	// physics
	BallWorld::update(); // also does its sound, which may be an issue
	nanScan();

	// handle collisions (right after physics!)
	handleCollisions();
	nanScan();

	// goal spawn
	tickGoalSpawn();
	nanScan();
	
	updateBallData();
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
			mPd->sendBang("goal-spawn");
			// new goal!
			int n = 1;

			for( int i=0; i<mTuning.mMultigoalMax; ++i )
			{
				if (randInt()%mTuning.mMultigoalOdds==0) {
					n++;
				}
			}

			for ( int i=0; i<n; ++i ) {
				spawnGoal( getRandomPointInWorldBoundsPoly() );
			}

			mGoalBallSpawnWaitTicks = -1;
		}
	}
}

void RaceWorld::spawnGoal( vec2 loc )
{
	// pfx
	const float pr = mTuning.mGoalBallRadius * 2.f; 
	
	for( int i=0; i<10; ++i )
	{
		vec2  v = randVec2();
		
		makePfx(loc + pr * v,
				mTuning.mGoalBallRadius * randFloat(.5f,.8f),
				lerp( mTuning.mGoalBallColor, ColorA(1,1,1,1), randFloat() ),
				-v * .1f,
				true );
	}	

	
	//
	Ball b;

	b.mColor  = mTuning.mGoalBallColor;
	b.mRibbonColor = mTuning.mGoalBallColor;
	b.mRadius = mTuning.mGoalBallRadius;
	b.mHistory.set_capacity( getRibbonMaxLength() );
	b.setLoc( loc );
	b.setVel( randVec2() * randFloat() * mTuning.mGoalBallSpawnMaxVel );

	auto ballData = make_shared<BallData>();
	ballData->mType = BallData::Type::Goal;
	ballData->mAttractAnim = 0.f;
	b.mUserData = ballData;
	getBalls().push_back(b);

	mGoalCount++;
}

void RaceWorld::handleCollisions()
{
	const vector<Ball>& balls = getBalls();
	set<size_t> removeBall;

	// ball-world and ball-contour collisions
	{
		auto bwc = getBallWorldCollisions();
		auto bcc = getBallContourCollisions();
		
		auto test = [this,&balls,&removeBall]( int i, vec2 pt )
		{
			auto bd = getBallData( balls[i] );
			if (!bd) return;
			
			// hit wall/world
			switch( bd->mType )
			{
				case BallData::Type::Shot:
				{
					// DINK!
					if ((1)) {
						removeBall.insert(i);
						
						makePfx(balls[i].mLoc,
								balls[i].mRadius * 2.f,
								lerp( balls[i].mColor, ColorA(1,1,1,1), .5f ),
								balls[i].getVel() * .01f + randVec2() * .01f,
								false );
								
						for( int i=0; i<10; ++i )
						{
							makePfx(pt,
									mTuning.mPfxCollideDustRadius * randFloat(1.f,3.f),
									lerp( mTuning.mPfxCollideDustColor, balls[i].mColor, randFloat() ),
									balls[i].getVel() * .01f + randVec2() * .2f,
									true );
						}
						
					} else {
						Ball& shot = getBalls()[i];
						shot.mPausePhysics  = 1;
						shot.mCollisionMask = 0;
						bd->mFadeOut = true;
					}

					FX("shot hit world/wall");
				}
				break;

				case BallData::Type::Player:
				{
					makePfx(pt,
							mTuning.mPfxCollideDustRadius * randFloat(1.f,2.5f),
							mTuning.mPfxCollideDustColor,
							balls[i].getVel() * .01f + randVec2() * .1f,
							false );
				}
				break;
				
				case BallData::Type::Goal:
				{
					for( int i=0; i<10; ++i )
					{
						makePfx(pt,
								mTuning.mPfxCollideDustRadius * randFloat(1.f,2.f),
								mTuning.mPfxCollideDustColor,
								balls[i].getVel() * .01f + randVec2() * .3f,
								false );
					}					
				}
				break;
				
				default:break;
			}
		};

		for( auto c : bcc ) test(c.mBallIndex,c.mPt);
		for( auto c : bwc ) test(c.mBallIndex,c.mPt);
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
			
			assert( c.mBallIndex[0] != c.mBallIndex[1] );

			
			{
				vec2 pt = lerp(
					getBalls()[ c.mBallIndex[0] ].mLoc,
					getBalls()[ c.mBallIndex[1] ].mLoc,
					.5f ); // TODO: have ballworld tell us!
					
				for( int i=0; i<10; ++i )
				{
					makePfx(pt,
							mTuning.mPfxCollideDustRadius,
							mTuning.mPfxCollideDustColor,
							  balls[c.mBallIndex[0]].getVel() * .1f
							+ balls[c.mBallIndex[1]].getVel() * .1f
							+ randVec2() * .3f,
							false );
				}					
			}
			
			// Player hits X
			// 0 isa player
			// 1 isa X
			if ( d[0]->mType == BallData::Type::Player )
			{
				Player* p = getPlayerByGamepad( d[0]->mGamepad );

				switch( d[1]->mType )
				{
					// goal
					case BallData::Type::Goal:
					{
						Ball g = getBalls()[c.mBallIndex[1]];
						removeBall.insert(c.mBallIndex[1]);

						// score it
						if (p) p->mScore++;
						FX("get goal");
						mPd->sendFloat("ship-goal", p->mScore);
						
						// pfx it
						for( int i=0; i<10; ++i )
						{
							makePfx(g.mLoc,
									g.mRadius * randFloat(.5f,1.8f),
									lerp( g.mColor, ColorA(1,1,1,1), randFloat() ),
									randVec2() * randFloat(.2f,.5f),
									true );
						}
					}
					break;

					// bullet
					case BallData::Type::Shot:
					{
						// no self-kill
						if ( d[0]->mGamepad != d[1]->mGamepad )
						{
							// decay shot
							getBalls()[c.mBallIndex[1]].mPausePhysics  = 1;
							getBalls()[c.mBallIndex[1]].mCollisionMask = 0;
							d[1]->mFadeOut = true;
							
							// kaboom
							//removeBall.insert(c.mBallIndex[1]); // shot
							
							// 
							removeBall.insert(c.mBallIndex[0]); // player
							mPd->sendBang("ship-explode");
							FX("player die");

							// pfx
							Ball pb = getBalls()[c.mBallIndex[0]];
							
							for( int i=0; i<10; ++i )
							{
								makePfx(pb.mLoc,
										mTuning.mPfxCollideDustRadius * randFloat(2.f,8.f),
										lerp( pb.mColor, ColorA(1,1,1,1), randFloat()*randFloat() ),
										randVec2() * randFloat(.3f,2.f),
										true );
							}	

							
							// update player
							if (p)
							{
								Ball pb = getBalls()[p->mBallIndex];
								p->mBallIndex = -1;
								p->mSpawnWait = mTuning.mPlayerRespawnWaitTicks;

								int spawnGoals = roundf( (float)p->mScore * mTuning.mPlayerDieSpawnGoalToScoreFrac );
								spawnGoals = max( 1, spawnGoals );

								p->mScore = 0;

								for( int i=0; i<spawnGoals; ++i )
								{
									spawnGoal( pb.mLoc + randVec2() * pb.mRadius );
								}
							}
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
			} // player hits X
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
//	gl::ScopedBlend blendScp( GL_SRC_ALPHA, GL_ONE );

	//
	// Draw world
	//
	BallWorld::draw(drawType);

	for( auto &p : mPlayers ) {
		drawPlayer(p.second);
	}
	
	drawBallData();
}

void RaceWorld::updateBallData()
{
	for( auto &b : getBalls() )
	{
		auto bd = getBallData(b);
		if (!bd) continue;

		cout << mTuning.mAttractAnimLength << endl;
		bd->mAttractAnim += 1.f / (60.f * mTuning.mAttractAnimLength);
	}
}

void RaceWorld::drawBallData() const
{
	for( auto &b : getBalls() )
	{
		auto bd = getBallData(b);
		if (!bd) continue;
		if ( bd->mAttractAnim >= 1.f ) continue;
		
		ColorA c = b.mColor;
		c.a *= sqrtf(1.f - bd->mAttractAnim);
		gl::color(c);

		bool isPlayer = bd->mType == BallData::Type::Player;
		int n = isPlayer ? 10 : 5 ;
		
		for( int i=0; i<n; ++i )
		{
			float r = b.mRadius * 3.f + (float)i * 2.f;
			r = lerp( r, b.mRadius + (float)i * .5f, bd->mAttractAnim );
			
			gl::drawStrokedCircle( b.mLoc, r );
		}
	}
}

void RaceWorld::nanScan()
{
	if (!kNanScan) return;
	
	for( auto b : getBalls() )
	{
		if ( isnan(b.mAccel.x) ) {
			cout << "nan" << b.mAccel << endl;
		}

		if ( isnan(b.mLoc.x) ) {
			cout << "nan" << b.mLoc << endl;
		}

		if ( isnan(b.mLastLoc.x) ) {
			cout << "nan" << b.mLastLoc << endl;
		}
	}
}

void RaceWorld::dumpCore()
{
	for ( auto pi : mPlayers )
	{
		const Player &p = pi.second;
		
		cout << "Player" << endl;
		cout << "\tmFacing = " << p.mFacing << endl;
		cout << "\tmScore  = " << p.mScore << endl;
		cout << "\tmBallIndex = " << p.mBallIndex << endl;
	}

	for ( int i=0; i<getBalls().size(); ++i )
	{
		const Ball& b = getBalls()[i]; 
		
		cout << "Ball[" << i << "]: loc=" << b.mLoc << ", vel=" << b.getVel() << endl;
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
		app->hotloadableAssetPath("synths/RaceWorld/race-world.pd"),
		app->hotloadableAssetPath("synths/RaceWorld/fm-voice-2.pd"),
		app->hotloadableAssetPath("synths/RaceWorld/spaceship-engine.pd"),
	};

	// Register file-watchers for all the major pd patch components
	mFileWatch.load( paths, [this,app]()
					{
						// Reload the root patch
						auto rootPatch = app->hotloadableAssetPath("synths/RaceWorld/race-world.pd");
						mPd->closePatch(mPatch);
						mPatch = mPd->loadPatch( DataSourcePath::create(rootPatch) ).get();
					});
}

RaceWorld::~RaceWorld() {
	// Close pong synthesis patch
	mPd->closePatch(mPatch);
}
