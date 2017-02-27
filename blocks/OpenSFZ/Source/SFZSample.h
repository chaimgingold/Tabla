#ifndef SFZSample_h
#define SFZSample_h

#include "SFZCommon.h"
#include "SFZAudioReader.h"

class SFZSample {
	public:
    SFZSample(const std::string& fileIn)
			: loopStart(0), loopEnd(0), fileName(fileIn), buffer(NULL), sampleRate(44100.0f) {}
		SFZSample(double sampleRateIn)
			: sampleLength(0), loopStart(0), loopEnd(0),
			  buffer(NULL), sampleRate(sampleRateIn) {}
		~SFZSample();

		bool	load();

		SFZAudioBuffer*	getBuffer() { return buffer; }
		double	getSampleRate() { return sampleRate; }
        std::string	getShortName();
		void	setBuffer(SFZAudioBuffer* newBuffer);
		SFZAudioBuffer*	detachBuffer();
		void	dump();

		unsigned long	sampleLength, loopStart, loopEnd;

#ifdef DEBUG
		void	checkIfZeroed(const char* where);
#endif

	protected:
        std::string fileName;
		SFZAudioBuffer*	buffer;
		double	sampleRate;
    
        SFZAudioReader loader;
	};


#endif 	// !SFZSample_h

