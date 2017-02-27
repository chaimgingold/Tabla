//
//  Synthesizer.cpp
//  OpenSFZ
//
//  Created by David Wallin on 4/22/13.
//  Copyright (c) 2013 David Wallin. All rights reserved.
//

#include "Synthesizer.h"

SynthesizerSound::SynthesizerSound()
{
}

SynthesizerSound::~SynthesizerSound()
{
}

/////////////////////////////////////////////////////////////////////////////////////

SynthesizerVoice::SynthesizerVoice()
{
    sampleRate = 44100.0f;

    currentlyPlayingNote = -1;
    noteOnTime = 0;
    currentlyPlayingSound = 0;
    keyIsDown = false; // the voice may still be playing when the key is not down (i.e. sustain pedal)
    sostenutoPedalDown = false;
}

void SynthesizerVoice::setCurrentPlaybackSampleRate (double newRate)
{
	sampleRate = newRate;
    
}

void SynthesizerVoice::clearCurrentNote()
{
    currentlyPlayingNote = -1;
    currentlyPlayingSound = NULL;
}


/////////////////////////////////////////////////////////////////////////////////////

Synthesizer::Synthesizer()
{
	sampleRate = 44100.0f;
    noteStealingEnabled = false;
    lastNoteOnCounter = 0;
    
    for(int i=0; i<16; i++)
        lastPitchWheelValues[i] = 0x2000;
}

Synthesizer::~Synthesizer()
{
	clearVoices();
	clearSounds();
}

void Synthesizer::addVoice (SynthesizerVoice* newVoice)
{
	voices.push_back(newVoice);
}

void Synthesizer::clearVoices()
{
	for(int i=0; i<voices.size(); i++)
		delete voices[i];

	voices.clear();
}
	
SynthesizerVoice* Synthesizer::getVoice (int index) const
{
	if(index < 0 || index >= voices.size())
		return NULL;

	return voices[index];
}

    
void Synthesizer::clearSounds()
{
	for(int i=0; i<sounds.size(); i++)
		delete sounds[i];

	voices.clear();
}

void Synthesizer::addSound (SynthesizerSound *newSound)
{
    sounds.push_back(newSound);
}

/** Removes and deletes one of the sounds. */
void Synthesizer::removeSound (int index)
{
    delete sounds[index];
    
    sounds.erase(sounds.begin() + index);
}


void Synthesizer::noteOn (int midiChannel,
                        int midiNoteNumber,
                        float velocity)
{

    
	for(int i=0; i<sounds.size(); i++)
	{
		SynthesizerSound *sound = sounds[i];


            // If hitting a note that's still ringing, stop it first (it could be
            // still playing because of the sustain or sostenuto pedal).
            for (int j = 0; j<voices.size(); j++)
            {
                SynthesizerVoice* const voice = voices[j];

                if (voice->getCurrentlyPlayingNote() == midiNoteNumber)
                    stopVoice (voice, true);
            }
        
            startVoice (findFreeVoice (sound, noteStealingEnabled),
                        sound, midiChannel, midiNoteNumber, velocity);
	}

}
    
void Synthesizer::noteOff (int midiChannel,
                        int midiNoteNumber,
                        bool allowTailOff)
{
	// not worth it since we override this
    
    for (int i = voices.size()-1; i >= 0; i-- )
    {
        SynthesizerVoice* const voice = voices[i];
        
        if (voice->getCurrentlyPlayingNote() == midiNoteNumber)
        {
            SynthesizerSound* const sound = voice->getCurrentlyPlayingSound();
            
            if (sound != NULL)
            {
                voice->keyIsDown = false;
                
            //    if (! (sustainPedalsDown [midiChannel] || voice->sostenutoPedalDown))
                    stopVoice (voice, allowTailOff);
            }
        }
    }
}
    
void Synthesizer::allNotesOff (int midiChannel,
                            bool allowTailOff)
{

    for (int i = 0; i<voices.size(); i++)
    {
        SynthesizerVoice* const voice = voices[i];


        voice->stopNote (allowTailOff);
    }
}
    
void Synthesizer::handlePitchWheel (int midiChannel,
                                int wheelValue)
{

    for (int i=0; i<voices.size(); i++)
    {
        SynthesizerVoice* const voice = voices[i];


        voice->pitchWheelMoved (wheelValue);
    }
}

void Synthesizer::handleController (int midiChannel,
                                int controllerNumber,
                                int controllerValue)
{
    switch (controllerNumber)
    {
        //case 0x40:  handleSustainPedal   (midiChannel, controllerValue >= 64); break;
        //case 0x42:  handleSostenutoPedal (midiChannel, controllerValue >= 64); break;
        //case 0x43:  handleSoftPedal      (midiChannel, controllerValue >= 64); break;
        default:    break;
    }


    for (int i=0; i<voices.size(); i++)
    {
        SynthesizerVoice* const voice = voices[i];

        voice->controllerMoved (controllerNumber, controllerValue);
    }
}

void Synthesizer::setCurrentPlaybackSampleRate (double sampleRate_)
{
	sampleRate = sampleRate_;
    
	for(int i=0; i<voices.size(); i++)
	{
        voices[i]->setCurrentPlaybackSampleRate(sampleRate_);
    }
    
}

SynthesizerVoice* Synthesizer::findFreeVoice (SynthesizerSound* soundToPlay,
                                            const bool stealIfNoneAvailable) const
{
	for(int i=0; i<voices.size(); i++)
	{
		if(voices[i]->getCurrentlyPlayingNote() == -1)
		{
			return voices[i];
		}
	}
    if (stealIfNoneAvailable)
    {
		SynthesizerVoice *oldest = NULL;
		for(int i=0; i<voices.size(); i++)
		{
            SynthesizerVoice* const voice = voices[i];
            
            if(oldest == NULL || oldest->noteOnTime > voice->noteOnTime)
                oldest = voice;
		}

		if(oldest)
			return oldest;
	}

	return NULL;
}

void Synthesizer::startVoice (SynthesizerVoice* voice,
                    SynthesizerSound* sound,
                    int midiChannel,
                    int midiNoteNumber,
                    float velocity)
{
    if (voice != NULL && sound != NULL)
    {
        if (voice->currentlyPlayingSound != NULL)
            voice->stopNote (false);
        
        voice->startNote (midiNoteNumber, velocity, sound,
                          lastPitchWheelValues [midiChannel - 1]);
        
        voice->currentlyPlayingNote = midiNoteNumber;
        voice->noteOnTime = ++lastNoteOnCounter;
        voice->currentlyPlayingSound = sound;
        voice->keyIsDown = true;
        voice->sostenutoPedalDown = false;
    }

}

void Synthesizer::stopVoice (SynthesizerVoice* voice, const bool allowTailOff)
{
    //assert (voice != NULL);
    
    voice->stopNote (allowTailOff);
    
    // the subclass MUST call clearCurrentNote() if it's not tailing off! RTFM for stopNote()!
    //assert (allowTailOff || (voice->getCurrentlyPlayingNote() < 0 && voice->getCurrentlyPlayingSound() == 0));
}

void Synthesizer::renderNextBlock (SFZAudioBuffer& outputAudio,
                        int startSample,
                        int numSamples)
{
    
    for (int i=0; i<voices.size(); i++)
        voices[i]->renderNextBlock (outputAudio, startSample, numSamples);


}