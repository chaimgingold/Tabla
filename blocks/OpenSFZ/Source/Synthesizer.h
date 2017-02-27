//
//  Synthesizer.h
//  OpenSFZ
//
//  Created by David Wallin on 4/22/13.
//  Copyright (c) 2013 David Wallin. All rights reserved.
//

#ifndef __OpenSFZ__Synthesizer__
#define __OpenSFZ__Synthesizer__

#include "SFZCommon.h"
class SynthesizerSound;

class SynthesizerVoice {
public:
    SynthesizerVoice();
    
    void setCurrentPlaybackSampleRate (double newRate);
    void clearCurrentNote();
    int getCurrentlyPlayingNote() { return currentlyPlayingNote; };
    SynthesizerSound *getCurrentlyPlayingSound() const            { return currentlyPlayingSound; };

    
    virtual void startNote (const int midiNoteNumber,
                            const float velocity,
                            SynthesizerSound* sound,
                            const int currentPitchWheelPosition) = 0;
    
    virtual void stopNote (const bool allowTailOff) = 0;
    virtual void pitchWheelMoved (const int newValue) = 0;
    virtual void controllerMoved (const int controllerNumber,
                                  const int newValue) = 0;
    
    
    //==============================================================================
    /** Renders the next block of data for this voice.
     
     The output audio data must be added to the current contents of the buffer provided.
     Only the region of the buffer between startSample and (startSample + numSamples)
     should be altered by this method.
     
     If the voice is currently silent, it should just return without doing anything.
     
     If the sound that the voice is playing finishes during the course of this rendered
     block, it must call clearCurrentNote(), to tell the synthesiser that it has finished.
     
     The size of the blocks that are rendered can change each time it is called, and may
     involve rendering as little as 1 sample at a time. In between rendering callbacks,
     the voice's methods will be called to tell it about note and controller events.
     */
    virtual void renderNextBlock (SFZAudioBuffer& outputBuffer,
                                  int startSample,
                                  int numSamples) = 0;
    
protected:
    double sampleRate;
    
    friend class Synthesizer;
    
    int currentlyPlayingNote;
    uint32_t noteOnTime;
    SynthesizerSound *currentlyPlayingSound;
    bool keyIsDown; // the voice may still be playing when the key is not down (i.e. sustain pedal)
    bool sostenutoPedalDown;
    

};

class SynthesizerSound {
public:
	SynthesizerSound();
    virtual ~SynthesizerSound();
    
    
};

class Synthesizer {
public:
	Synthesizer();
	virtual ~Synthesizer();
    void addVoice (SynthesizerVoice* newVoice);
	
    SynthesizerVoice* getVoice (int index) const;

    int getNumVoices() const          
	{ 
		return (int)voices.size();
	}

	void clearVoices();
    
    
    //==============================================================================
    /** Deletes all sounds. */
    void clearSounds();
    
    /** Returns the number of sounds that have been added to the synth. */
    int getNumSounds() const                                        { return sounds.size(); }
    
    /** Returns one of the sounds. */
    SynthesizerSound* getSound (int index) const                    { return sounds [index]; }
    
    
    void addSound (SynthesizerSound *newSound);
    
    /** Removes and deletes one of the sounds. */
    void removeSound (int index);

    
    
    virtual void noteOn (int midiChannel,
                         int midiNoteNumber,
                         float velocity);
    
    virtual void noteOff (int midiChannel,
                          int midiNoteNumber,
                          bool allowTailOff);
    
    virtual void allNotesOff (int midiChannel,
                              bool allowTailOff);
    
    virtual void handlePitchWheel (int midiChannel,
                                   int wheelValue);
    virtual void handleController (int midiChannel,
                                   int controllerNumber,
                                   int controllerValue);
    void setCurrentPlaybackSampleRate (double sampleRate);
    
    virtual SynthesizerVoice* findFreeVoice (SynthesizerSound* soundToPlay,
                                             const bool stealIfNoneAvailable) const;
    void startVoice (SynthesizerVoice* voice,
                     SynthesizerSound* sound,
                     int midiChannel,
                     int midiNoteNumber,
                     float velocity);
    void stopVoice (SynthesizerVoice* voice, const bool allowTailOff);

    void renderNextBlock (SFZAudioBuffer& outputAudio,
                          int startSample,
                          int numSamples);
public:
    double sampleRate;
    uint32_t lastNoteOnCounter;
    std::vector<SynthesizerVoice *>voices;
    std::vector<SynthesizerSound *>sounds;
    
    /** The last pitch-wheel values for each midi channel. */
    int lastPitchWheelValues [16];
    
protected:
    bool noteStealingEnabled;

};

#endif /* defined(__OpenSFZ__Synthesizer__) */
