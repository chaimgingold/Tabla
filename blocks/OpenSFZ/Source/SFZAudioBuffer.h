//
//  SFZAudioBuffer.h
//  OpenSFZ
//
//  Created by David Wallin on 4/22/13.
//  Copyright (c) 2013 David Wallin. All rights reserved.
//
//  Audio buffer class for loading samples and generating output. Currently only supports
//  1 or 2 channels.

#ifndef __OpenSFZ__SFZAudioBuffer__
#define __OpenSFZ__SFZAudioBuffer__


class SFZAudioBuffer
{
public:
    SFZAudioBuffer(const int numChannels_, const unsigned long numSamples_);
    
    SFZAudioBuffer(const unsigned long numSamples_, float *channel1, float *channel2);
    SFZAudioBuffer(const SFZAudioBuffer &other);
    ~SFZAudioBuffer();
    
    float *channels[2];
    
    unsigned long getNumSamples() { return numSamples; };
    int getNumChannels() { return numChannels; };
private:
    unsigned long numSamples;
    int numChannels;
    
    bool owned;

};

         




#endif /* defined(__OpenSFZ__SFZAudioBuffer__) */
