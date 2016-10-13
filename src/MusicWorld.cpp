//
//  MusicWorld.cpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 9/20/16.
//
//

#include "MusicWorld.h"
#include "geom.h"
#include "cinder/rand.h"
#include "cinder/audio/Context.h"
#include "cinder/audio/Source.h"

using namespace std::chrono;

MusicWorld::MusicWorld()
{
	mStartTime = ci::app::getElapsedSeconds();

//	setupSynthesis();
//	cg: pd code seems to be causing crashes, but same code as PongWorld. (???)
}

void MusicWorld::update()
{
}

void MusicWorld::draw( bool highQuality )
{
	//
	// Draw field layout markings
	//
	if (0)
	{
		gl::color(0,.8,0,1.f);
		
		PolyLine2 wb = getWorldBoundsPoly();
		wb.setClosed();
		gl::draw( wb );

	}
}

// Synthesis
void MusicWorld::setupSynthesis()
{
	auto ctx = audio::master();
	
	// Create the synth engine
	mPureDataNode = ctx->makeNode( new cipd::PureDataNode( audio::Node::Format().autoEnable() ) );
	
	// Enable Cinder audio
	ctx->enable();
	
	// Load pong synthesis
	mPureDataNode->disconnectAll();
	mPatch = mPureDataNode->loadPatch( DataSourcePath::create(getAssetPath("synths/pong.pd")) );
	
	// Connect synth to master output
	mPureDataNode >> audio::master()->getOutput();
}

MusicWorld::~MusicWorld() {
	// Clean up synth engine
//	mPureDataNode->disconnectAll();
//	mPureDataNode->closePatch(mPatch);
}