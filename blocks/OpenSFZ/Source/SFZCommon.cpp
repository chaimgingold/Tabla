//
//  SFZCommon.cpp
//  OpenSFZ
//
//  Created by David Wallin on 4/25/13.
//  Copyright (c) 2013 David Wallin. All rights reserved.
//

#include "SFZCommon.h"

float decibelsToGain(float db)
{
    return powf(10.0f, db / 20.0f);
}

float getMidiNoteInHertz(int noteNumber)
{
    noteNumber -= 12 * 6 + 9; // now 0 = A
    return 440.0f * pow (2.0, noteNumber / 12.0);
}