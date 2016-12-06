//
//  PinballWorld.cpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 12/5/16.
//
//

#include "PinballWorld.h"
#include "geom.h"
#include "cinder/rand.h"
#include "cinder/audio/Context.h"
#include "cinder/audio/Source.h"
#include "Gamepad.h"


PinballWorld::PinballWorld()
{
	setupSynthesis();
	
	mGamepadManager.mOnButtonDown = []( const GamepadManager::Event& event )
	{
		cout << "down" << endl;
	};

	mGamepadManager.mOnButtonUp = []( const GamepadManager::Event& event )
	{
		cout << "up" << endl;
	};
	
	// inputs
	mInputToFunction["flippers-left"] = []()
	{
		cout << "flippers-left" << endl;
	};

	mInputToFunction["flippers-right"] = []()
	{
		cout << "flippers-right" << endl;
	};
}

PinballWorld::~PinballWorld()
{
	shutdownSynthesis();
}

void PinballWorld::setParams( XmlTree xml )
{
	BallWorld::setParams(xml);
	
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

void PinballWorld::gameWillLoad()
{
	// most important thing is to prevent BallWorld from doing its default thing.
}

void PinballWorld::update()
{
	mGamepadManager.tick();

	BallWorld::update();
	
	if ( getBalls().size() < 2 )
	{
		newRandomBall( getRandomPointInWorldBoundsPoly() ).mCollideWithContours = true;
	}
}

void PinballWorld::onBallBallCollide   ( const Ball&, const Ball& )
{
	mPureDataNode->sendBang("new-game");

	if (0) cout << "ball ball collide" << endl;
}

void PinballWorld::onBallContourCollide( const Ball&, const Contour& )
{
	mPureDataNode->sendBang("hit-object");

	if (0) cout << "ball contour collide" << endl;
}

void PinballWorld::onBallWorldBoundaryCollide	( const Ball& b )
{
	mPureDataNode->sendBang("hit-wall");

	if (0) cout << "ball world collide" << endl;
}

void PinballWorld::draw( DrawType drawType )
{

	//
	// Draw world
	//
	BallWorld::draw(drawType);
	
}

void PinballWorld::worldBoundsPolyDidChange()
{
}

void PinballWorld::keyDown( KeyEvent event )
{
	char c = event.getChar();
	
	auto i = mKeyToInput.find(c);
	
	if (i!=mKeyToInput.end())
	{
		auto j = mInputToFunction.find(i->second);
		if (j!=mInputToFunction.end())
		{
			j->second();
		}
	}
}

// Synthesis
void PinballWorld::setupSynthesis()
{
	mPureDataNode = cipd::PureDataNode::Global();
	
	// Load pong synthesis patch
	mPatch = mPureDataNode->loadPatch( DataSourcePath::create(getAssetPath("synths/pong.pd")) );
}

void PinballWorld::shutdownSynthesis() {
	// Close pong synthesis patch
	mPureDataNode->closePatch(mPatch);
}