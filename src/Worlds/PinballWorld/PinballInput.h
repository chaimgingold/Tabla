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
	bool isFlipperDown( int side ) const { assert(side==0||side==1); return mIsFlipperDown[side]; }
	bool isPaused() const { return mPauseBallWorld; }
	
private:
	PinballWorld& mWorld;

	// are flippers depressed
	bool mIsFlipperDown[2]; // left, right	
	bool mPauseBallWorld=false;
	
	// generic
	void setupControls();
	map<string,function<void()>> mInputToFunction; // maps input names to code handlers
	void processInputEvent( string name );
	
	// keyboard map
	map<char,string> mKeyToInput; // maps keystrokes to input names
	void processKeyEvent( KeyEvent, string suffix );

	// game pad
	GamepadManager mGamepadManager;
	map<unsigned int,string> mGamepadButtons;
};


}

#endif /* PinballInput_hpp */
