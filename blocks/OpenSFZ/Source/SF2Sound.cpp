#include "SF2Sound.h"
#include "SF2Reader.h"
#include "SFZSample.h"



SF2Sound::SF2Sound(const Path& file)
	: SFZSound(file)
{
}


SF2Sound::~SF2Sound()
{
	// "presets" owns the regions, so clear them out of "regions" so ~SFZSound()
	// doesn't try to delete them.
	regions.clear();

	// The samples all share a single buffer, so make sure they don't all delete it.
	SFZAudioBuffer* buffer = NULL;
	for (std::map<unsigned long, SFZSample*>::iterator iter = samplesByRate.begin(); iter != samplesByRate.end(); iter++)
		buffer = iter->second->detachBuffer();
	delete buffer;
}


class PresetComparator {
	public:
		static int compareElements(const SF2Sound::Preset* first, const SF2Sound::Preset* second) {
			int cmp = first->bank - second->bank;
			if (cmp != 0)
				return cmp;
			return first->preset - second->preset;
			}
	};

void SF2Sound::loadRegions()
{
	SF2Reader reader(this, file);
	reader.read();

	// Sort the presets.
	//PresetComparator comparator;
	//presets.sort(comparator);

	useSubsound(0);
}


void SF2Sound::loadSamples(
	double* progressVar)
{
	SF2Reader reader(this, file);
	SFZAudioBuffer* buffer = reader.readSamples(progressVar);
	if (buffer)
    {
		// All the SFZSamples will share the buffer.
        std::map<unsigned long, SFZSample*>::iterator iter;
        
		for (iter = samplesByRate.begin(); iter != samplesByRate.end(); iter++)
			iter->second->setBuffer(buffer);
    }

	if (progressVar)
		*progressVar = 1.0;
}


void SF2Sound::addPreset(SF2Sound::Preset* preset)
{
	presets.push_back(preset);
}


int SF2Sound::numSubsounds()
{
	return presets.size();
}


std::string SF2Sound::subsoundName(int whichSubsound)
{
	Preset* preset = presets[whichSubsound];
	std::string result;
	if (preset->bank != 0) {
		result += preset->bank;
		result += "/";
		}
	result += preset->preset;
	result += ": ";
	result += preset->name;
	return result;
}


void SF2Sound::useSubsound(int whichSubsound)
{
	selectedPreset = whichSubsound;
	regions.clear();
    
    for(int i=0; i<presets[whichSubsound]->regions.size(); i++)
        addRegion(presets[whichSubsound]->regions[i]);

}


int SF2Sound::selectedSubsound()
{
	return selectedPreset;
}


SFZSample* SF2Sound::sampleFor(unsigned long sampleRate)
{
	SFZSample* sample = samplesByRate[sampleRate];
	if (sample == NULL) {
		sample = new SFZSample(sampleRate);
		samplesByRate[sampleRate] = sample;
		}
	return sample;
}


void SF2Sound::setSamplesBuffer(SFZAudioBuffer* buffer)
{
    
    std::map<unsigned long, SFZSample*>::iterator iter;
    
    for (iter = samplesByRate.begin(); iter != samplesByRate.end(); iter++)
        iter->second->setBuffer(buffer);
}



