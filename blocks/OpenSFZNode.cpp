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

using namespace ci;

OpenSFZNode::OpenSFZNode(const Format &format): Node{ format } {
	if (getChannelMode() != ChannelMode::SPECIFIED) {
		setChannelMode(ChannelMode::SPECIFIED);
		setNumChannels(2);
	}
}

void OpenSFZNode::initialize() {
	mSynth = new SFZSynth();
	mSynth->setCurrentPlaybackSampleRate(getSampleRate());


	fs::path path = TablaApp::get()->hotloadableAssetPath( fs::path("samples/Virtual-Playing-Orchestra/Strings/harp-sustain.sfz"));
	SFZSound *testSound = new SFZSound(path.string());

	mSynth->addSound(testSound);

	testSound->loadRegions();
	testSound->loadSamples();

	testSound->dump();    // print out debug info
}

void OpenSFZNode::uninitialize() {

}

void OpenSFZNode::process(audio::Buffer *buffer) {
	// Audio processing. Call something like this in your audio processing callback loop:
	// Note: audio buffers should be filled with zeros first
	size_t numSamples = getFramesPerBlock();
	SFZAudioBuffer sfzBuffer((int)numSamples,
						  buffer->getChannel(0),
						  buffer->getChannel(1));

	mSynth->renderNextBlock(sfzBuffer,
							0,
							(int)numSamples);
}
