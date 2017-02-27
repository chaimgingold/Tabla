#ifndef SFZSound_h
#define SFZSound_h

#include "SFZCommon.h"
#include "SFZRegion.h"
#include "Synthesizer.h"

class SFZSample;

// File = path to .SFZ file?
class SFZSound : public SynthesizerSound {
	public:
		SFZSound(const Path& file);
        SFZSound(std::string path);
		virtual ~SFZSound();

		bool	appliesToNote(const int midiNoteNumber);
		bool	appliesToChannel(const int midiChannel);

		void	addRegion(SFZRegion* region); 	// Takes ownership of the region.
        SFZSample*	addSample(std::string path, std::string defaultPath = std::string(""));
		void	addError(const std::string& message);
		void	addUnsupportedOpcode(const std::string& opcode);


		virtual void	loadRegions();
		virtual void	loadSamples(double* progressVar = NULL);

		SFZRegion*	getRegionFor(
			int note, int velocity, SFZRegion::Trigger trigger = SFZRegion::attack);
		int	getNumRegions();
		SFZRegion*	regionAt(int index);

		std::string	getErrorsString();

		virtual int	numSubsounds();
		virtual std::string	subsoundName(int whichSubsound);
		virtual void	useSubsound(int whichSubsound);
		virtual int 	selectedSubsound();

		void	dump();

	protected:
    void setFile(const Path &p);
		Path 	file;
        std::vector<SFZRegion*>	regions;
        std::map<std::string, SFZSample*>	samples;
        std::vector<std::string>      	errors;
        std::map<std::string, std::string>	unsupportedOpcodes;
	};



#endif 	// SFZSound_h

