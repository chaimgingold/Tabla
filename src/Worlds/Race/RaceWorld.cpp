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
	setupGamepadManager();
}

void RaceWorld::gameWillLoad()
{
	// most important thing is to prevent BallWorld from doing its default thing. (makes balls)
}

void RaceWorld::worldBoundsPolyDidChange()
{
	// most important thing is to prevent BallWorld from doing its default thing. (makes balls)
}

void RaceWorld::setupGamepadManager()
{
//	auto gamepadManager = getGamepadManager(); 	
	auto gamepadManager = mGamepadManager; 	

	gamepadManager.mOnButtonDown = [this]( const GamepadManager::Event& event )
	{
		setupGamepad(event.mDevice);

		cout << "down " << event.mId << endl;
//		button(event.mId,true);
	};

	gamepadManager.mOnButtonUp = [this]( const GamepadManager::Event& event )
	{
		setupGamepad(event.mDevice);

		cout << "up "  << event.mId << endl;
//		button(event.mId,false);
	};
	
	gamepadManager.mOnAxisMoved = [this]( const GamepadManager::Event& event )
	{
		if (0) {
			cout << "axis " << event.mId << ": " << event.mAxisValue << endl;
		}
	};

	gamepadManager.mOnDeviceAttached = [this]( const GamepadManager::Event& event )
	{
		cout << "attached "  << event.mDevice << endl;
	};
	
	gamepadManager.mOnDeviceRemoved = [this]( const GamepadManager::Event& event )
	{
		cout << "removed "  << event.mDevice << endl;
		removePlayer(event.mDevice);
	};
}

void RaceWorld::setupGamepad( Gamepad_device* gamepad )
{
	if ( gamepad && mPlayers.find(gamepad->deviceID) == mPlayers.end() )
	{
		Player p;
	
		newRandomBall( getRandomPointInWorldBoundsPoly() );
	
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
	mGamepadManager.tick();

	BallWorld::update(); // also does its sound, which may be an issue
	
	/*while ( getBalls().size() < 5 )
	{
		newRandomBall( getRandomPointInWorldBoundsPoly() );
	}*/
}

void RaceWorld::draw( DrawType drawType )
{
	mFileWatch.update();
	
	//
	// Draw world
	//
	BallWorld::draw(drawType);
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
