#ifndef SFZEG_h
#define SFZEG_h

#include "SFZRegion.h"


class SFZEG {
	public:
		constexpr static const float bottomLevel = 0.001;

		SFZEG();

		void	setExponentialDecay(bool newExponentialDecay);
		void	startNote(
			const SFZEGParameters* parameters, float floatVelocity, float sampleRate,
			const SFZEGParameters* velMod = 0);
		void	nextSegment();
		void	noteOff();
		void	fastRelease();
		bool	isDone() { return segment == Done; }
		bool	isReleasing() { return segment == Release; }
		int 	segmentIndex() { return (int) segment; }

		float	level;
		float	slope;
		long 	samplesUntilNextSegment;
		bool	segmentIsExponential;

	protected:
		enum Segment {
			Delay, Attack, Hold, Decay, Sustain, Release, Done
			};
		Segment	segment;
		SFZEGParameters	parameters;
		float	sampleRate;
		bool	exponentialDecay;

		void	startDelay();
		void	startAttack();
		void	startHold();
		void	startDecay();
		void	startSustain();
		void	startRelease();
	};


#endif 	// !SFZEG_h

