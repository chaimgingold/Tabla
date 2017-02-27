#include "SFZCommon.h"

#include "SFZVoice.h"
#include "SFZSound.h"
#include "SFZRegion.h"
#include "SFZSample.h"
#include <math.h>
#include <assert.h>

static const float globalGain = -1.0;


SFZVoice::SFZVoice()
	: region(NULL)
{
    trigger = 0;
    
    curMidiNote = 0;
    curPitchWheel = 0;
    pitchRatio = 1.0;
    noteGainLeft = 1.0;
    noteGainRight = 1.0;
    sourceSamplePosition = 0.0f;
    sampleEnd = 0;
    loopStart = 0, loopEnd = 0;
    
    // Info only.
    numLoops = 0;
    curVelocity = 0;
    
	ampeg.setExponentialDecay(true);
}


SFZVoice::~SFZVoice()
{
}


bool SFZVoice::canPlaySound(SynthesizerSound* sound)
{
	return dynamic_cast<SFZSound*>(sound) != NULL;
}


void SFZVoice::startNote(
	const int midiNoteNumber,
	const float floatVelocity,
	SynthesizerSound* soundIn,
	const int currentPitchWheelPosition)
{
	SFZSound* sound = dynamic_cast<SFZSound*>(soundIn);
	if (sound == NULL) {
		killNote();
		return;
		}

	int velocity = (int) (floatVelocity * 127.0);
	curVelocity = velocity;
	if (region == NULL)
		region = sound->getRegionFor(midiNoteNumber, velocity);
	if (region == NULL || region->sample == NULL || region->sample->getBuffer() == NULL) {
		killNote();
		return;
		}
	if (region->negative_end) {
		killNote();
		return;
		}

	// Pitch.
	curMidiNote = midiNoteNumber;
	curPitchWheel = currentPitchWheelPosition;
	calcPitchRatio();

	// Gain.
	double noteGainDB = globalGain + region->volume;
	// Thanks to <http:://www.drealm.info/sfz/plj-sfz.xhtml> for explaining the
	// velocity curve in a way that I could understand, although they mean
	// "log10" when they say "log".
	double velocityGainDB = -20.0 * log10((127.0 * 127.0) / (velocity * velocity));
	velocityGainDB *= region->amp_veltrack / 100.0;
	noteGainDB += velocityGainDB;
	noteGainLeft = noteGainRight = decibelsToGain(noteGainDB);
	// The SFZ spec is silent about the pan curve, but a 3dB pan law seems
	// common.  This sqrt() curve matches what Dimension LE does; Alchemy Free
	// seems closer to sin(adjustedPan * pi/2).
	double adjustedPan = (region->pan + 100.0) / 200.0;
	noteGainLeft *= sqrt(1.0 - adjustedPan);
	noteGainRight *= sqrt(adjustedPan);
	ampeg.startNote(
		&region->ampeg, floatVelocity, sampleRate, &region->ampeg_veltrack);

	// Offset/end.
	sourceSamplePosition = region->offset;
	sampleEnd = region->sample->sampleLength;
	if (region->end > 0 && region->end < sampleEnd)
		sampleEnd = region->end + 1;

	// Loop.
	loopStart = loopEnd = 0;
	SFZRegion::LoopMode loopMode = region->loop_mode;
	if (loopMode == SFZRegion::sample_loop) {
		if (region->sample->loopStart < region->sample->loopEnd)
			loopMode = SFZRegion::loop_continuous;
		else
			loopMode = SFZRegion::no_loop;
		}
	if (loopMode != SFZRegion::no_loop && loopMode != SFZRegion::one_shot) {
		if (region->loop_start < region->loop_end) {
			loopStart = region->loop_start;
			loopEnd = region->loop_end;
			}
		else {
			loopStart = region->sample->loopStart;
			loopEnd = region->sample->loopEnd;
			}
		}
	numLoops = 0;
}


void SFZVoice::stopNote(const bool allowTailOff)
{
	if (!allowTailOff || region == NULL) {
		killNote();
		return;
		}

	if (region->loop_mode != SFZRegion::one_shot)
		ampeg.noteOff();
	if (region->loop_mode == SFZRegion::loop_sustain) {
		// Continue playing, but stop looping.
		loopEnd = loopStart;
		}
}


void SFZVoice::stopNoteForGroup()
{
	if (region->off_mode == SFZRegion::fast)
		ampeg.fastRelease();
	else
		ampeg.noteOff();
}


void SFZVoice::stopNoteQuick()
{
	ampeg.fastRelease();
}


void SFZVoice::pitchWheelMoved(const int newValue)
{
	if (region == NULL)
		return;

	curPitchWheel = newValue;
	calcPitchRatio();
}


void SFZVoice::controllerMoved(
	const int controllerNumber,
	const int newValue)
{
	/***/
}


void SFZVoice::renderNextBlock(
	SFZAudioBuffer& outputBuffer, int startSample, int numSamples)
{
	if (region == NULL)
		return;

	SFZAudioBuffer* buffer = region->sample->getBuffer();
	const float* inL = buffer->channels[0];
	const float* inR = buffer->getNumChannels() > 1 ? buffer->channels[1] : NULL;

	float* outL = outputBuffer.channels[0] + startSample;
	float* outR =
		outputBuffer.getNumChannels() > 1 ?
		outputBuffer.channels[1] + startSample : NULL;

	// Cache some values, to give them at least some chance of ending up in
	// registers.
	double sourceSamplePosition = this->sourceSamplePosition;
	float ampegGain = ampeg.level;
	float ampegSlope = ampeg.slope;
	long samplesUntilNextAmpSegment = ampeg.samplesUntilNextSegment;
	bool ampSegmentIsExponential = ampeg.segmentIsExponential;
	float loopStart = this->loopStart;
	float loopEnd = this->loopEnd;
	float sampleEnd = this->sampleEnd;

	while (--numSamples >= 0) {
		int pos = (int) sourceSamplePosition;
        
        assert(pos >= 0 && pos < buffer->getNumSamples());
        
		float alpha = (float) (sourceSamplePosition - pos);
		float invAlpha = 1.0f - alpha;
		int nextPos = pos + 1;
		if (loopStart < loopEnd && nextPos > loopEnd)
			nextPos = loopStart;

		// Simple linear interpolation.
		float l = (inL[pos] * invAlpha + inL[nextPos] * alpha);
		float r = inR ? (inR[pos] * invAlpha + inR[nextPos] * alpha) : l;

		float gainLeft = noteGainLeft * ampegGain;
		float gainRight = noteGainRight * ampegGain;
		l *= gainLeft;
		r *= gainRight;
		// Shouldn't we dither here?

		if (outR) {
			*outL++ += l;
			*outR++ += r;
        }
		else
			*outL++ += (l + r) * 0.5f;

		// Next sample.
		sourceSamplePosition += pitchRatio;
		if (loopStart < loopEnd && sourceSamplePosition > loopEnd) {
			sourceSamplePosition = loopStart;
			numLoops += 1;
			}

		// Update EG.
		if (ampSegmentIsExponential)
			ampegGain *= ampegSlope;
		else
			ampegGain += ampegSlope;
		if (--samplesUntilNextAmpSegment < 0) {
			ampeg.level = ampegGain;
			ampeg.nextSegment();
			ampegGain = ampeg.level;
			ampegSlope = ampeg.slope;
			samplesUntilNextAmpSegment = ampeg.samplesUntilNextSegment;
			ampSegmentIsExponential = ampeg.segmentIsExponential;
			}

		if (sourceSamplePosition >= sampleEnd || ampeg.isDone()) {
			killNote();
			break;
			}
		}

	this->sourceSamplePosition = sourceSamplePosition;
	ampeg.level = ampegGain;
	ampeg.samplesUntilNextSegment = samplesUntilNextAmpSegment;
}


bool SFZVoice::isPlayingNoteDown()
{
	return (region && region->trigger != SFZRegion::release);
}


bool SFZVoice::isPlayingOneShot()
{
	return (region && region->loop_mode == SFZRegion::one_shot);
}


int SFZVoice::getGroup()
{
	return (region ? region->group : 0);
}


int SFZVoice::getOffBy()
{
	return (region ? region->off_by : 0);
}


void SFZVoice::setRegion(SFZRegion* nextRegion)
{
	region = nextRegion;
}


std::string SFZVoice::infoString()
{
	const char* egSegmentNames[] = {
		"delay", "attack", "hold", "decay", "sustain", "release", "done"
		};
	#define numEGSegments (sizeof(egSegmentNames) / sizeof(egSegmentNames[0]))
	const char* egSegmentName = "-Invalid-";
	int egSegmentIndex = ampeg.segmentIndex();
	if (egSegmentIndex >= 0 && egSegmentIndex < numEGSegments)
		egSegmentName = egSegmentNames[egSegmentIndex];

	char str[128];
	snprintf(
		str, 128,
		"note: %d, vel: %d, pan: %g, eg: %s, loops: %lu",
		curMidiNote, curVelocity, region->pan, egSegmentName, numLoops);
	return str;
}



void SFZVoice::calcPitchRatio()
{
	double note = curMidiNote;
	note += region->transpose;
	note += region->tune / 100.0;

	double adjustedPitch =
		region->pitch_keycenter +
		(note - region->pitch_keycenter) * (region->pitch_keytrack / 100.0);
	if (curPitchWheel != 8192) {
		double wheel = ((2.0 * curPitchWheel / 16383.0) - 1.0);
		if (wheel > 0)
			adjustedPitch += wheel * region->bend_up / 100.0;
		else
			adjustedPitch += wheel * region->bend_down / -100.0;
		}
	double targetFreq = noteHz(adjustedPitch);
	double naturalFreq = getMidiNoteInHertz(region->pitch_keycenter);
	pitchRatio =
		(targetFreq * region->sample->getSampleRate()) /
		(naturalFreq * sampleRate);
}


void SFZVoice::killNote()
{
	region = NULL;
	clearCurrentNote();
}


double SFZVoice::noteHz(double note, const double freqOfA)
{
	// Like MidiMessage::getMidiNoteInHertz(), but with a float note.
	note -= 12 * 6 + 9;
	// Now 0 = A
	return freqOfA * pow(2.0, note / 12.0);
}



