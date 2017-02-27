#ifndef SF2Reader_h
#define SF2Reader_h

#include "SF2.h"

class SF2Sound;
class SFZRegion;
class SFZSample;


class SF2Reader {
	public:
		SF2Reader(SF2Sound* sound, const Path& p);
		~SF2Reader();

		void	read();
		SFZAudioBuffer*	readSamples(
			double* progressVar = NULL);

	protected:
		SF2Sound*	sound;
		InputStream*	file;

		void	addGeneratorToRegion(
			word genOper, SF2::genAmountType* amount, SFZRegion* region);
	};

#endif 	// !SF2Reader_h

