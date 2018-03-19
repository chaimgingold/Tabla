//
//  SFZAudioBuffer.cpp
//  OpenSFZ
//
//  Created by David Wallin on 4/22/13.
//  Copyright (c) 2013 David Wallin. All rights reserved.
//

#include "SFZAudioBuffer.h"


SFZAudioBuffer::SFZAudioBuffer(const int numChannels_, const unsigned long numSamples_)
{
    channels[0] = 0;
    channels[1] = 0;
    
    numSamples = numSamples_;
    
    if( numSamples < 0 || numSamples > 158760000)
    {
        owned = false;
        channels[0] = 0;
        return;
    }
    
    int c = numChannels_;
    
    if(c > 2)
        c = 2;
    
    numChannels = c;
    
    for(int i=0; i<numChannels; i++)
    {
        channels[i] = new float[numSamples + 5];
        
        for(unsigned long j=0; j< numSamples + 5; j++)
            channels[i][j] = 0.0f;
    }
    owned = true;
}

SFZAudioBuffer::SFZAudioBuffer(const SFZAudioBuffer &other)
{
    owned = false;
    
    numChannels = other.numChannels;
    numSamples = other.numSamples;
    
    for(int i=0; i<numChannels; i++)
    {
        channels[i] = other.channels[i];
    }
    
}

SFZAudioBuffer::~SFZAudioBuffer()
{
    if(owned)
    {
        for(int i=0; i<numChannels; i++)
        {
            delete channels[i];
            channels[i] = 0;
        }
    }
}

SFZAudioBuffer::SFZAudioBuffer(const unsigned long numSamples_, float *channel1, float *channel2)
{
    owned = false;
    numChannels = 2;
    numSamples = numSamples_;
    channels[0] = channel1;
    channels[1] = channel2;
}
