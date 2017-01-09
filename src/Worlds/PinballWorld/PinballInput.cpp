//
//  PinballInput.cpp
//  Tabla
//
//  Created by Chaim Gingold on 1/8/17.
//
//

#include "PinballInput.h"
#include "PinballWorld.h"
#include "xml.h"

namespace Pinball
{

PinballInput::PinballInput( PinballWorld& world ) : mWorld(world)
{
	mIsFlipperDown[0] = false;
	mIsFlipperDown[1] = false;	
	
	auto button = [this]( unsigned int id, string postfix )
	{
		auto i = mGamepadButtons.find(id);
		if (i!=mGamepadButtons.end())
		{			
			processInputEvent( i->second + postfix );
		}
	};
	
	
	mGamepadManager.mOnButtonDown = [this,button]( const GamepadManager::Event& event )
	{
		cout << "down " << event.mId << endl;
		button(event.mId,"-down");
	};

	mGamepadManager.mOnButtonUp = [this,button]( const GamepadManager::Event& event )
	{
		cout << "up "  << event.mId << endl;
		button(event.mId,"-up");
	};
	
	// inputs
	auto flipperChange = [this]( int side, int state )
	{
		bool oldState = mIsFlipperDown[side]; 
		
		mIsFlipperDown[side] = state;
		
		if ( state != oldState ) // i think we need this for repeat key-down events, but not gamepad
		{
			if (state) mWorld.serveBallIfNone();
			
			if ( mWorld.getPartCensus().getPop( flipperIndexToType(side) ) > 0 )
			{
				mWorld.getPd()->sendFloat("flipper-change", state);
				
				if (state) mWorld.doFlipperScreenShake();
			}
		}
	};
	
	mInputToFunction["flippers-left-down"]  = [this,flipperChange]() { flipperChange(0,1); };
	mInputToFunction["flippers-left-up"]    = [this,flipperChange]() { flipperChange(0,0); };
	mInputToFunction["flippers-right-down"] = [this,flipperChange]() { flipperChange(1,1); };
	mInputToFunction["flippers-right-up"]   = [this,flipperChange]() { flipperChange(1,0); };
	mInputToFunction["pause-ball-world-down"]    = [this](){ mPauseBallWorld = !mPauseBallWorld; };
	mInputToFunction["serve-multiball-down"]     = [this](){ mWorld.serveBallCheat(); }; 
}

void PinballInput::setParams( XmlTree xml )
{
	// gamepad
	mGamepadButtons.clear();
	if (xml.hasChild("Gamepad"))
	{
		XmlTree keys = xml.getChild("Gamepad");
		
		for( auto item = keys.begin("button"); item != keys.end(); ++item )
		{
			if (item->hasAttribute("id") && item->hasAttribute("do"))
			{
				unsigned int id = item->getAttributeValue<unsigned int>("id");
				string _do = item->getAttributeValue<string>("do");
				
				cout << id << " -> " << _do << endl;
				
				mGamepadButtons[id] = _do;
			}
		}
	}
	
	// keyboard
	mKeyToInput.clear();
	if (xml.hasChild("KeyMap"))
	{
		XmlTree keys = xml.getChild("KeyMap");
		
		for( auto item = keys.begin("Key"); item != keys.end(); ++item )
		{
			if ( item->hasChild("char") && item->hasChild("input") )
			{
				string	charkey	= item->getChild("char").getValue<string>();
				string	input	= item->getChild("input").getValue();
				
				char ckey = charkey.front();
				
				cout << ckey << ", " << input << endl;

				mKeyToInput[ckey] = input;
			}
		}
	}
}

void PinballInput::tick()
{
	mGamepadManager.tick();
}

void PinballInput::processInputEvent( string name )
{
	cout << "-> " << name << endl;

	auto j = mInputToFunction.find(name);
	if (j!=mInputToFunction.end())
	{
		j->second();
	}
}

void PinballInput::processKeyEvent( KeyEvent event, string suffix )
{
	char c = event.getChar();
	
	auto i = mKeyToInput.find(c);
	
	if (i!=mKeyToInput.end())
	{
		processInputEvent(i->second + suffix);
	}
}

void PinballInput::keyDown( KeyEvent event )
{
	processKeyEvent(event, "-down");
}

void PinballInput::keyUp( KeyEvent event )
{
	processKeyEvent(event, "-up");
}

}