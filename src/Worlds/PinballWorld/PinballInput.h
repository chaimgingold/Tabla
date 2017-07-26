//
//  PinballInput.hpp
//  Tabla
//
//  Created by Chaim Gingold on 1/8/17.
//
//

#ifndef PinballInput_hpp
#define PinballInput_hpp

#include "cinder/app/KeyEvent.h"
#include "cinder/Xml.h"
#include "GamepadManager.h"
#include <map>
#include <string>

namespace Pinball
{

class PinballWorld;

using namespace ci;
using namespace ci::app;
using namespace std;

class PinballInput
{
public:
	PinballInput( PinballWorld& );

	void setParams( XmlTree );

	void tick();
	void keyDown( KeyEvent );
	void keyUp( KeyEvent );
	void gamepadEvent( const GamepadManager::Event& );
	bool isFlipperDown( int side ) const { assert(side==0||side==1); return mIsFlipperDown[side]; }
	bool isPaused() const { return mPauseBallWorld; }
	float getPlungerState() const { return mPlungerState; }
	// TODO: Plunger: Add a release trigger/event, with strike force.
	
private:
	PinballWorld& mWorld;

	// parsed input
	bool  mIsFlipperDown[2]; // left, right: are flippers depressed	
	bool  mPauseBallWorld=false;
	int   mIsPlungerKeyDown=0; // -1: up, 0: none, 1: down
	float mPlungerState=0.f;
	
	// generic
	void setupControls();
	map<string,function<void( float state )>> mInputToFunction; // maps input names to code handlers
	void processInputEvent( string name, float state );
	
	// keyboard map
	map<char,string> mKeyToInput; // maps keystrokes to input names
	void processKeyEvent( KeyEvent, bool down );

	// game pad
//	GamepadManager mGamepadManager;
	map<unsigned int,string> mGamepadButtons;
	map<unsigned int,string> mGamepadAxes;
	map<string,vector<unsigned int>> mAxisIdForAction;
	
	float reduceAxisForAction( string actionName, float v1, function<float(float,float)> reduce );
	
	bool mGamepadVerboseAxes=false;
};


}

#endif /* PinballInput_hpp */
