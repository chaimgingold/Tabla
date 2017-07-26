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

	void gameWillLoad() override;
	void worldBoundsPolyDidChange() override;
	void update() override;
	void draw( DrawType ) override;

protected:

private:
	FileWatch mFileWatch;

	GamepadManager mGamepadManager;
	
	class Player
	{
	public:
		GamepadManager::DeviceId mGamepad=0; // for convenience
		int mBallIndex=-1;
	};
	
	typedef map<GamepadManager::DeviceId,Player> tDeviceToPlayerMap;
	tDeviceToPlayerMap mPlayers;

	void setupGamepadManager();
	void setupGamepad( Gamepad_device* ); // idempotent
	void removePlayer( Gamepad_device* );
		
	// synthesis
	cipd::PureDataNodeRef	mPd;	// synth engine
	cipd::PatchRef			mPatch;			// pong patch
	
	void setupSynthesis() override;
	void updateSynthesis() override {};
};

#endif /* RaceWorld_hpp */
