#ifndef SFZRegion_h
#define SFZRegion_h

#include "SFZCommon.h"

class SFZSample;


// SFZRegion is designed to be able to be bitwise-copied.

struct SFZEGParameters {
	float	delay, start, attack, hold, decay, sustain, release;

	void	clear();
	void	clearMod();
	};

class SFZRegion {
	public:
		enum Trigger {
			attack, release, first, legato
			};
		enum LoopMode {
			sample_loop, no_loop, one_shot, loop_continuous, loop_sustain
			};
		enum OffMode {
			fast, normal
			};


		SFZRegion();
		void	clear();
		void	clearForSF2();
		void	clearForRelativeSF2();
		void	addForSF2(SFZRegion* other);
		void	sf2ToSFZ();
		void	dump();

		bool	matches(unsigned char note, unsigned char velocity, Trigger trigger) {
			return
				note >= lokey && note <= hikey &&
				velocity >= lovel && velocity <= hivel &&
				(trigger == this->trigger ||
				 (this->trigger == attack && (trigger == first || trigger == legato)));
			}

		SFZSample* sample;
		unsigned char lokey, hikey;
		unsigned char lovel, hivel;
		Trigger trigger;
		unsigned long group, off_by;
		OffMode off_mode;

		unsigned long offset;
		unsigned long end;
		bool negative_end;
		LoopMode loop_mode;
		unsigned long loop_start, loop_end;
		int transpose;
		int tune;
		int pitch_keycenter, pitch_keytrack;
		int bend_up, bend_down;

		float volume, pan;
		float amp_veltrack;

		SFZEGParameters	ampeg, ampeg_veltrack;

		static float	timecents2Secs(short timecents);
	};


#endif 	// !SFZRegion_h

