#ifndef SFZVoice_h
#define SFZVoice_h

#include "SFZCommon.h"
#include "SFZEG.h"
#include "SFZAudioBuffer.h"
#include "Synthesizer.h"

class SFZRegion;


class SFZVoice : public SynthesizerVoice {
	public:
		SFZVoice();
		~SFZVoice();

    bool	canPlaySound(SynthesizerSound* sound);
    void	startNote(
			const int midiNoteNumber,
			const float velocity,
			SynthesizerSound* sound,
			const int currentPitchWheelPosition);
    void	stopNote(const bool allowTailOff);
		void	stopNoteForGroup();
		void	stopNoteQuick();
    void	pitchWheelMoved(const int newValue);
    void	controllerMoved(
			const int controllerNumber,
			const int newValue);
    void	renderNextBlock(
			SFZAudioBuffer& outputBuffer, int startSample, int numSamples);
		bool	isPlayingNoteDown();
		bool	isPlayingOneShot();

		int	getGroup();
		int	getOffBy();

		// Set the region to be used by the next startNote().
		void	setRegion(SFZRegion* nextRegion);

		std::string	infoString();

	protected:
		int       	trigger;
		SFZRegion*	region;
		int       	curMidiNote, curPitchWheel;
		double    	pitchRatio;
		float     	noteGainLeft, noteGainRight;
		double    	sourceSamplePosition;
		SFZEG     	ampeg;
		unsigned long	sampleEnd;
		unsigned long	loopStart, loopEnd;

		// Info only.
		unsigned long	numLoops;
		int	curVelocity;

		void	calcPitchRatio();
		void	killNote();
		double	noteHz(double note, const double freqOfA = 440.0);
	};


#endif 	// SFZVoice_h

