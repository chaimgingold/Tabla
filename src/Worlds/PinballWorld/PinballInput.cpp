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
	
	auto button = [this]( unsigned int id, bool down )
	{
		auto i = mGamepadButtons.find(id);
		if (i!=mGamepadButtons.end())
		{			
			processInputEvent( i->second, down ? 1.f : 0.f );
		}
	};
	
	
	mGamepadManager.mOnButtonDown = [this,button]( const GamepadManager::Event& event )
	{
		cout << "down " << event.mId << endl;
		button(event.mId,true);
	};

	mGamepadManager.mOnButtonUp = [this,button]( const GamepadManager::Event& event )
	{
		cout << "up "  << event.mId << endl;
		button(event.mId,false);
	};
	
	mGamepadManager.mOnAxisMoved = [this]( const GamepadManager::Event& event )
	{
		if ( mGamepadVerboseAxes &&
		    (mGamepadAxes.empty() || mGamepadAxes.find(event.mId) != mGamepadAxes.end() )
		   )
		{
			cout << "axis " << event.mId << ": " << event.mAxisValue << endl;
		}
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
	
	mInputToFunction["flippers-left"]    = [this,flipperChange]( float down ) { flipperChange(0,down); };
	mInputToFunction["flippers-left"]    = [this,flipperChange]( float down ) { flipperChange(0,down); };
	mInputToFunction["flippers-right"]   = [this,flipperChange]( float down ) { flipperChange(1,down); };
	mInputToFunction["flippers-right"]   = [this,flipperChange]( float down ) { flipperChange(1,down); };
	mInputToFunction["pause-ball-world"] = [this]( float down ){ if (down) mPauseBallWorld = !mPauseBallWorld; };
	mInputToFunction["serve-multiball"]  = [this]( float down ){ if (down) mWorld.serveBallCheat(); };
}

void PinballInput::setParams( XmlTree xml )
{
	const bool kVerbose = false;
	
	// gamepad
	getXml(xml, "GamepadVerboseAxes", mGamepadVerboseAxes);
	
	mGamepadButtons.clear();
	mGamepadAxes.clear();
	if (xml.hasChild("Gamepad"))
	{
		XmlTree keys = xml.getChild("Gamepad");
		
		auto read = [keys]( string name, map<unsigned int,string>& mapping )
		{
			for( auto item = keys.begin(name); item != keys.end(); ++item )
			{
				if (item->hasAttribute("id") && item->hasAttribute("do"))
				{
					unsigned int id = item->getAttributeValue<unsigned int>("id");
					string _do = item->getAttributeValue<string>("do");
					
					if (kVerbose) cout << id << " -> " << _do << endl;
					
					mapping[id] = _do;
				}
			}
		};
		
		read("button",mGamepadButtons);
		read("axis",  mGamepadAxes);
	}
	
	// keyboard
	mKeyToInput.clear();
	if (xml.hasChild("Keys"))
	{
		XmlTree keys = xml.getChild("Keys");
		
		for( auto item = keys.begin("key"); item != keys.end(); ++item )
		{
			if ( item->hasAttribute("char") && item->hasAttribute("do") )
			{
				string	charkey	= item->getAttributeValue<string>("char");
				string	input	= item->getAttributeValue<string>("do");
				
				char ckey = charkey.front();
				
				if (kVerbose) cout << ckey << ", " << input << endl;

				mKeyToInput[ckey] = input;
			}
		}
	}
}

void PinballInput::tick()
{
	mGamepadManager.tick();
}

void PinballInput::processInputEvent( string name, float state )
{
	cout << "-> " << name << " " << state << endl;

	auto j = mInputToFunction.find(name);
	if (j!=mInputToFunction.end())
	{
		j->second(state);
	}
}

void PinballInput::processKeyEvent( KeyEvent event, bool down )
{
	char c = event.getChar();
	
	auto i = mKeyToInput.find(c);
	
	if (i!=mKeyToInput.end())
	{
		processInputEvent( i->second, down ? 1.f : 0.f );
	}
}

void PinballInput::keyDown( KeyEvent event )
{
	processKeyEvent(event,true);
}

void PinballInput::keyUp( KeyEvent event )
{
	processKeyEvent(event,false);
}

}