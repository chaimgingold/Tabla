//
//  OpenSFZ.h
//  OpenSFZ
//
//  Created by David Wallin on 4/22/13.
//  Copyright (c) 2013 David Wallin. All rights reserved.
//

#ifndef OpenSFZ_SFZCommon_h
#define OpenSFZ_SFZCommon_h

#include <string>
#include <vector>
#include <map>

#include <stdio.h>

#include <fstream>
#include <sstream>

#include <math.h>
#include "SFZAudioBuffer.h"
#include "Path.h"

#include "Synthesizer.h"


#define VORBIS 1

extern float decibelsToGain(float db);
extern float getMidiNoteInHertz(int noteNumber);

//



#endif
