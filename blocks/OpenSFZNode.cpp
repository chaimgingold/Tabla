//
//  OpenSFZNode.cpp
//  Tabla
//
//  Created by Luke Iannini on 2/28/17.
//
//

#include "OpenSFZNode.h"

#include "cinder/Log.h"
#include "cinder/audio/Context.h"
#include "cinder/audio/dsp/Converter.h"
#include "TablaApp.h"

OpenSFZNode::OpenSFZNode(const Format &format): Node{ format } {
	if (getChannelMode() != ChannelMode::SPECIFIED) {
		setChannelMode(ChannelMode::SPECIFIED);
		setNumChannels(2);
	}
}

void OpenSFZNode::initialize() {
	mSynth = new SFZSynth();
	mSynth->setCurrentPlaybackSampleRate(getSampleRate());
}

OpenSFZNodeRef OpenSFZNode::initWithSound(fs::path path) {
	auto ctx = audio::master();


	// Create a SoundFont sample player
	OpenSFZNodeRef sfzNode = ctx->makeNode(new OpenSFZNode( audio::Node::Format().autoEnable() ));

	// Connect to output
	sfzNode >> audio::master()->getOutput();


	fs::path fullpath = TablaApp::get()->hotloadableAssetPath(path);
	sfzNode->mSound = new SFZSound(fullpath.string());
	sfzNode->mSound->loadRegions();
	sfzNode->mSound->loadSamples();

	sfzNode->mSynth->addSound(sfzNode->mSound);

	//	sound->dump();    // print out debug info

	return sfzNode;
}


void OpenSFZNode::uninitialize() {
	delete mSynth;
	delete mSound;
}

void OpenSFZNode::process(audio::Buffer *buffer) {
	size_t numSamples = getFramesPerBlock();
	SFZAudioBuffer sfzBuffer((int)numSamples,
						  buffer->getChannel(0),
						  buffer->getChannel(1));

	mSynth->renderNextBlock(sfzBuffer,
							0,
							(int)numSamples);
}
