#include "SFZEG.h"
#include <math.h>

static const float fastReleaseTime = 0.01;


SFZEG::SFZEG()
	: exponentialDecay(false)
{
}


void SFZEG::setExponentialDecay(bool newExponentialDecay)
{
	exponentialDecay = newExponentialDecay;
}


void SFZEG::startNote(
	const SFZEGParameters* newParameters, float floatVelocity,
	float newSampleRate,
	const SFZEGParameters* velMod)
{
	parameters = *newParameters;
	if (velMod) {
		parameters.delay += floatVelocity * velMod->delay;
		parameters.attack += floatVelocity * velMod->attack;
		parameters.hold += floatVelocity * velMod->hold;
		parameters.decay += floatVelocity * velMod->decay;
		parameters.sustain += floatVelocity * velMod->sustain;
		if (parameters.sustain < 0.0)
			parameters.sustain = 0.0;
		else if (parameters.sustain > 100.0)
			parameters.sustain = 100.0;
		parameters.release += floatVelocity * velMod->release;
		}
	sampleRate = newSampleRate;

	startDelay();
}


void SFZEG::nextSegment()
{
	switch (segment) {
		case Delay:
			startAttack();
			break;

		case Attack:
			startHold();
			break;

		case Hold:
			startDecay();
			break;

		case Decay:
			startSustain();
			break;

		case Sustain:
			// Shouldn't be called.
			break;

		case Release:
		default:
			segment = Done;
			break;
		}
}


void SFZEG::noteOff()
{
	startRelease();
}


void SFZEG::fastRelease()
{
	segment = Release;
	samplesUntilNextSegment = fastReleaseTime * sampleRate;
	slope = -level / samplesUntilNextSegment;
	segmentIsExponential = false;
}


void SFZEG::startDelay()
{
	if (parameters.delay <= 0)
		startAttack();
	else {
		segment = Delay;
		level = 0.0;
		slope = 0.0;
		samplesUntilNextSegment = parameters.delay * sampleRate;
		segmentIsExponential = false;
		}
}


void SFZEG::startAttack()
{
	if (parameters.attack <= 0)
		startHold();
	else {
		segment = Attack;
		level = parameters.start / 100.0;
		samplesUntilNextSegment = parameters.attack * sampleRate;
		slope = 1.0 / samplesUntilNextSegment;
		segmentIsExponential = false;
		}
}


void SFZEG::startHold()
{
	if (parameters.hold <= 0) {
		level = 1.0;
		startDecay();
		}
	else {
		segment = Hold;
		samplesUntilNextSegment = parameters.hold * sampleRate;
		level = 1.0;
		slope = 0.0;
		segmentIsExponential = false;
		}
}


void SFZEG::startDecay()
{
	if (parameters.decay <= 0)
		startSustain();
	else {
		segment = Decay;
		samplesUntilNextSegment = parameters.decay * sampleRate;
		level = 1.0;
		if (exponentialDecay) {
			// I don't truly understand this; just following what LinuxSampler does.
			float mysterySlope = -9.226 / samplesUntilNextSegment;
			slope = exp(mysterySlope);
			segmentIsExponential = true;
			if (parameters.sustain > 0.0) {
				// Again, this is following LinuxSampler's example, which is similar to
				// SF2-style decay, where "decay" specifies the time it would take to
				// get to zero, not to the sustain level.  The SFZ spec is not that
				// specific about what "decay" means, so perhaps it's really supposed
				// to specify the time to reach the sustain level.
				samplesUntilNextSegment =
					(long) (log((parameters.sustain / 100.0) / level) / mysterySlope);
				if (samplesUntilNextSegment <= 0)
					startSustain();
				}
			}
		else {
			slope = (parameters.sustain / 100.0 - 1.0) / samplesUntilNextSegment;
			segmentIsExponential = false;
			}
		}
}


void SFZEG::startSustain()
{
	if (parameters.sustain <= 0)
		startRelease();
	else {
		segment = Sustain;
		level = parameters.sustain / 100.0;
		slope = 0.0;
		samplesUntilNextSegment = 0x7FFFFFFF;
		segmentIsExponential = false;
		}
}


void SFZEG::startRelease()
{
	float release = parameters.release;
	if (release <= 0) {
		// Enforce a short release, to prevent clicks.
		release = fastReleaseTime;
		}

	segment = Release;
	samplesUntilNextSegment = release * sampleRate;
	if (exponentialDecay) {
		// I don't truly understand this; just following what LinuxSampler does.
		float mysterySlope = -9.226 / samplesUntilNextSegment;
		slope = exp(mysterySlope);
		segmentIsExponential = true;
		}
	else {
		slope = -level / samplesUntilNextSegment;
		segmentIsExponential = false;
		}
}



