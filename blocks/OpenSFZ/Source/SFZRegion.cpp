#include "SFZRegion.h"
#include "SFZSample.h"
#include <string.h>
#include <stdio.h>


void SFZEGParameters::clear()
{
	delay = 0.0;
	start = 0.0;
	attack = 0.0;
	hold = 0.0;
	decay = 0.0;
	sustain = 100.0;
	release = 0.0;
}


void SFZEGParameters::clearMod()
{
	// Clear for velocity or other modification.
	delay = start = attack = hold = decay = sustain = release = 0.0;
}



SFZRegion::SFZRegion()
{
	clear();
}


void SFZRegion::clear()
{
	memset(this, 0, sizeof(*this));
	hikey = 127;
	hivel = 127;
	pitch_keycenter = 60; 	// C4
	pitch_keytrack = 100;
	bend_up = 200;
	bend_down = -200;
	volume = pan = 0.0;
	amp_veltrack = 100.0;
	ampeg.clear();
	ampeg_veltrack.clearMod();
}


void SFZRegion::clearForSF2()
{
	clear();
	pitch_keycenter = -1;
	loop_mode = no_loop;

	// SF2 defaults in timecents.
	ampeg.delay = -12000.0;
	ampeg.attack = -12000.0;
	ampeg.hold = -12000.0;
	ampeg.decay = -12000.0;
	ampeg.sustain = 0.0;
	ampeg.release = -12000.0;
}


void SFZRegion::clearForRelativeSF2()
{
	clear();
	pitch_keytrack = 0;
	amp_veltrack = 0.0;
	ampeg.sustain = 0.0;
}


void SFZRegion::addForSF2(SFZRegion* other)
{
	offset += other->offset;
	end += other->end;
	loop_start += other->loop_start;
	loop_end += other->loop_end;
	transpose += other->transpose;
	tune += other->tune;
	pitch_keytrack += other->pitch_keytrack;
	volume += other->volume;
	pan += other->pan;

	ampeg.delay += other->ampeg.delay;
	ampeg.attack += other->ampeg.attack;
	ampeg.hold += other->ampeg.hold;
	ampeg.decay += other->ampeg.decay;
	ampeg.sustain += other->ampeg.sustain;
	ampeg.release += other->ampeg.release;
}


void SFZRegion::sf2ToSFZ()
{
	// EG times need to be converted from timecents to seconds.
	ampeg.delay = timecents2Secs(ampeg.delay);
	ampeg.attack = timecents2Secs(ampeg.attack);
	ampeg.hold = timecents2Secs(ampeg.hold);
	ampeg.decay = timecents2Secs(ampeg.decay);
	if (ampeg.sustain < 0.0)
		ampeg.sustain = 0.0;
	ampeg.sustain = 100.0 * decibelsToGain(-ampeg.sustain / 10.0);
	ampeg.release = timecents2Secs(ampeg.release);

	// Pin very short EG segments.  Timecents don't get to zero, and our EG is
	// happier with zero values.
	if (ampeg.delay < 0.01)
		ampeg.delay = 0.0;
	if (ampeg.attack < 0.01)
		ampeg.attack = 0.0;
	if (ampeg.hold < 0.01)
		ampeg.hold = 0.0;
	if (ampeg.decay < 0.01)
		ampeg.decay = 0.0;
	if (ampeg.release < 0.01)
		ampeg.release = 0.0;

	// Pin values to their ranges.
	if (pan < -100.0)
		pan = -100.0;
	else if (pan > 100.0)
		pan = 100.0;
}


void SFZRegion::dump()
{
	printf("%d - %d, vel %d - %d", lokey, hikey, lovel, hivel);
	if (sample) {
		printf(": %s", 		sample->getShortName().c_str());
		}
	printf("\n");
}


float SFZRegion::timecents2Secs(short timecents)
{
	return pow(2.0, timecents / 1200.0);
}



