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

PinballWorld::PinballWorld()
{
	
	setupSynthesis();
}

void PinballWorld::gameWillLoad()
{
	// most important thing is to prevent BallWorld from doing its default thing.
}

void PinballWorld::update()
{
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

// Synthesis
void PinballWorld::setupSynthesis()
{
	mPureDataNode = cipd::PureDataNode::Global();
	
	// Load pong synthesis patch
	mPatch = mPureDataNode->loadPatch( DataSourcePath::create(getAssetPath("synths/pong.pd")) );
}

PinballWorld::~PinballWorld() {
	// Close pong synthesis patch
	mPureDataNode->closePatch(mPatch);
}
