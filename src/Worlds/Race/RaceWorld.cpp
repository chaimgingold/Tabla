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
	setupSynthesis();
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
		ball.mCollideWithContours = true;
		ball.setVel( vec2(0,0) );
		ball.mAccel = vec2(0,0);
		
		p.mBallIndex = getBalls().size()-1; // assume new one is at the end :)
		p.mGamepad = gamepad->deviceID;
		
		mPlayers[gamepad->deviceID] = p;
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
}

void RaceWorld::update()
{
	// friction, accel
	for( auto &p : mPlayers )
	{
		auto &car = p.second; 
		
		if (car.mBallIndex!=-1)
		{
			Ball &ball = getBalls()[car.mBallIndex];
			
			const GamepadManager::Device* gamepad = getGamepadManager().getDeviceById(p.first);
			if (gamepad)
			{
				if (gamepad->buttonStates[0])
				{
					ball.mAccel += p.second.mFacing * .05f;
					// ideally we let the engine rev up and down so this happens smoothly.. (and feels a bit sloppy)
				}
				
				car.mFacing = glm::rotate( car.mFacing, gamepad->axisStates[0] * .08f );
			}
			
//			ball.mAccel = -.001f * ball.getVel();
			// TODO: do this in a more graceful way
		}
	}

	//
	BallWorld::update(); // also does its sound, which may be an issue
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
		if (p.second.mBallIndex!=-1)
		{
			Ball &ball = getBalls()[p.second.mBallIndex];
			auto &car = p.second; 
			
			gl::color(ball.mColor);
			gl::drawLine( ball.mLoc, ball.mLoc + ball.mRadius * 2.f * p.second.mFacing );
			
			gl::pushModelMatrix();
			gl::translate( ball.mLoc );
			gl::multModelMatrix( mat4( mat2( -perp(car.mFacing), car.mFacing ) ) );
			gl::scale( vec2(ball.mRadius) );

			gl::drawSolidRect( Rectf(-.5,-1,.5,1.5) );

			gl::popModelMatrix();
			
		}
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
