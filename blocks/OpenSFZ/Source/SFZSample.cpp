#include "SFZSample.h"

// get buffer directly from loader?

bool SFZSample::load()
{
    // A bunch of things needed here..
    
    // FIXME: check for wav or ogg..?
    loader.load(fileName);
    buffer = loader.buffer;
    sampleRate = loader.mySampleRate;
    sampleLength = loader.getLength();
    loopStart = loader.loopStart;
    loopEnd = loader.loopEnd;
    

    
    //loopStart = loader.loopStart;
    //loopEnd = loader.loopEnd;
    
    /*
	AudioFormatReader* reader = formatManager->createReaderFor(file);
	if (reader == NULL)
		return false;
	sampleRate = reader->sampleRate;
	sampleLength = reader->lengthInSamples;
	// Read some extra samples, which will be filled with zeros, so interpolation
	// can be done without having to check for the edge all the time.
	buffer = new SFZAudioBuffer(reader->numChannels, sampleLength + 4);
	reader->read(buffer, 0, sampleLength + 4, 0, true, true);
	StringPairArray* metadata = &reader->metadataValues;
	int numLoops = metadata->getValue("NumSampleLoops", "0").getIntValue();
	if (numLoops > 0) {
		loopStart = metadata->getValue("Loop0Start", "0").getLargeIntValue();
		loopEnd = metadata->getValue("Loop0End", "0").getLargeIntValue();
		}
	delete reader;
     */
    
	return true;
}


SFZSample::~SFZSample()
{

}


std::string SFZSample::getShortName()
{
    // FIXME: this isn't short.
    Path p(fileName);
    return p.getFileName();

}


void SFZSample::setBuffer(SFZAudioBuffer* newBuffer)
{
    if(buffer)
        delete buffer;
    
	buffer = newBuffer;
	sampleLength = buffer->getNumSamples();
}


SFZAudioBuffer* SFZSample::detachBuffer()
{
	SFZAudioBuffer* result = buffer;
	buffer = NULL;
	return result;
}


void SFZSample::dump()
{


	printf("%s\n %s\n", fileName.c_str(), loader.getSummary());
}


#ifdef DEBUG
void SFZSample::checkIfZeroed(const char* where)
{
	if (buffer == NULL) {
		printf("SFZSample::checkIfZeroed(%s): no buffer!", where);
		return;
		}

	int samplesLeft = buffer->getNumSamples();
	unsigned long nonzero = 0, zero = 0;
	float* p = buffer->channels[0];
	for (; samplesLeft > 0; --samplesLeft) {
		if (*p++ == 0.0)
			zero += 1;
		else
			nonzero += 1;
		}
	if (nonzero > 0)
		printf("Buffer not zeroed at %s (%lu vs. %lu).", where, nonzero, zero);
	else
		printf("Buffer zeroed at %s!  (%lu zeros)", where, zero);
}
#endif 	// JUCE_DEBUG



