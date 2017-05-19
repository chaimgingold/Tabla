//
//  OpenSFZNode.h
//  Tabla
//
//  Created by Luke Iannini on 2/28/17.
//
//

#ifndef OpenSFZNode_h
#define OpenSFZNode_h

#include <stdio.h>

#include "cinder/DataSource.h"
#include "cinder/Thread.h"
#include "cinder/audio/Node.h"

#include "OpenSFZ.h"

using namespace ci;

typedef std::shared_ptr<class OpenSFZNode> OpenSFZNodeRef;

class OpenSFZNode : public ci::audio::Node {
public:
	
	OpenSFZNode(const Format &format = Format());

	void initialize() override;
	void uninitialize() override;

	void process(ci::audio::Buffer *buffer) override;

	SFZSynth *mSynth;
	SFZSound *mSound;

	static OpenSFZNodeRef initWithSound(fs::path path);
};

#endif /* OpenSFZNode_h */
