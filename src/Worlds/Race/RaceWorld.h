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
	void gamepadEvent( const GamepadManager::Event& ) override;

protected:

private:
	FileWatch mFileWatch;

	class Tuning
	{
	public:
		bool  mShipDrawDebug = false;
	}
	mTuning;
	
	class Player
	{
	public:
		GamepadManager::DeviceId mGamepad=0; // for convenience
		int		mBallIndex=-1;
		vec2	mFacing=vec2(0,1);
		
		// graphics
//		ci::gl::TextureRef mImage;
	};
	
	typedef map<GamepadManager::DeviceId,Player> tDeviceToPlayerMap;
	tDeviceToPlayerMap mPlayers;

	void tickPlayer( Player& );
	void drawPlayer( const Player& ) const;
	
	void setupGamepad( Gamepad_device* ); // idempotent
	void removePlayer( Gamepad_device* );
	
	//
	ci::gl::TextureRef	mShip;
	float				mShipScale=1.f;
	
	// synthesis
	cipd::PureDataNodeRef	mPd;	// synth engine
	cipd::PatchRef			mPatch;			// pong patch
	
	void setupSynthesis() override;
	void updateSynthesis() override {};
};

#endif /* RaceWorld_hpp */
