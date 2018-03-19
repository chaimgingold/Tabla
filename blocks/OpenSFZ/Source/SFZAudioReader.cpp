//
//  SFZAudioReader.cpp
//  OpenSFZ
//
//  Created by David Wallin on 4/23/13.
//
//  This file originally comes from Maximillian
//  https://github.com/micknoise/Maximilian
//  but includes updates to read more types of files, loop points etc.
//
//  Utilizes the stb_vorbis code for ogg reading.
//  http://www.nothings.org/stb_vorbis/

#include <stdio.h>

#include "SFZAudioReader.h"
#include "stb_vorbis.h"

#include <assert.h>

using namespace std;

SFZAudioReader::SFZAudioReader()
: myData(NULL), position(0), recordPosition(0), myChannels(1), buffer(0), mySampleRate(44100.0f)
{
    length = 0;
    loopStart = 0;
    loopEnd = 0;
};


//This is the SFZAudioReader load function. It just calls read.
bool SFZAudioReader::load(string fileName) {
	myPath = fileName;

    if(buffer) {
        delete buffer;
        buffer = 0;
    }
    
    Path p(fileName);
    std::string ext = p.getExtension();
    if(ext == "ogg")
        return readOgg();
    else if(p.getExtension() == "wav")
        return readWav();
    
    return false;
}

// This is for OGG loading
bool SFZAudioReader::readOgg() {
#ifdef VORBIS
    bool result;
    short *temp;
    
    int channelx;
    //    cout << fileName << endl;
    myDataSize = stb_vorbis_decode_filename(const_cast<char*>(myPath.c_str()), &channelx, &temp);
    result = myDataSize > 0;
    
    printf("\nchannels = %d\nlength = %d",channelx,myDataSize);
    printf("\n");
    
    myChannels=(short)channelx;
    length=myDataSize;
    mySampleRate=44100;
    
    assert(myDataSize > 0);
    
    buffer = new SFZAudioBuffer(myChannels, myDataSize);
    
    if (myChannels>1) {
        int position=0;
        for (int i=0;i<myDataSize; i++) {
            for(int j=0; j<myChannels; j++)
            {
                buffer->channels[j][i] = (float)temp[position] / 32768.0f;
                position++;
            }
        }
    }
    
    delete temp;
    
	return result; // this should probably be something more descriptive
#endif
    return 0;
}


//This is the main read function.
bool SFZAudioReader::readWav()
{
	bool result;
	ifstream inFile( myPath.c_str(), ios::in | ios::binary);
	result = true;
	if (inFile) {
		bool datafound = false;
		inFile.seekg(4, ios::beg);
		inFile.read( (char*) &myChunkSize, 4 ); // read the ChunkSize
		
		inFile.seekg(16, ios::beg);
		inFile.read( (char*) &mySubChunk1Size, 4 ); // read the SubChunk1Size
		
		//inFile.seekg(20, ios::beg);
		inFile.read( (char*) &myFormat, sizeof(short) ); // read the file format.  This should be 1 for PCM
		
		//inFile.seekg(22, ios::beg);
		inFile.read( (char*) &myChannels, sizeof(short) ); // read the # of channels (1 or 2)
		
		//inFile.seekg(24, ios::beg);
		inFile.read( (char*) &mySampleRate, sizeof(int) ); // read the samplerate
		
		//inFile.seekg(28, ios::beg);
		inFile.read( (char*) &myByteRate, sizeof(int) ); // read the byterate
		
		//inFile.seekg(32, ios::beg);
		inFile.read( (char*) &myBlockAlign, sizeof(short) ); // read the blockalign
		
		//inFile.seekg(34, ios::beg);
		inFile.read( (char*) &myBitsPerSample, sizeof(short) ); // read the bitspersample
                
		
		//ignore any extra chunks
		char chunkID[5]="";
		chunkID[4] = 0;
		long filePos = 36;
        

        inFile.seekg (0, inFile.end);
        long fileLength = inFile.tellg();
        
        // !datafound &&
        
		while(filePos < fileLength && !inFile.eof()) {
            int chunkSize;
            
			inFile.seekg(filePos, ios::beg);
			inFile.read((char*) &chunkID, sizeof(char) * 4);
			inFile.seekg(filePos + 4, ios::beg);
			inFile.read( (char*) &chunkSize, sizeof(int) ); // read the size of the data
			filePos += 8;
			if (strcmp(chunkID,"data") == 0 && !datafound) {
                myDataSize = chunkSize;
                dataChunkPos = filePos;
				datafound = true;
			} else if (strcmp(chunkID,"smpl") == 0) {
                smplChunkSize = chunkSize;
                smplChunkPos = filePos;

                inFile.seekg(filePos, ios::beg);
                
                parseSMPLChunk(inFile, chunkSize);
                
                loopStart = sampleLoop.dwStart;
                loopEnd = sampleLoop.dwEnd;
            }
            
            
            filePos += chunkSize;

		}
        
        // FIXME: need to handle sample looping chunks, etc..
		
		// read the data chunk
		myData = (char*) malloc(myDataSize * sizeof(char));
		inFile.seekg(dataChunkPos, ios::beg);
		inFile.read(myData, myDataSize);
		length=myDataSize/((myBitsPerSample / 8) * myChannels);
		inFile.close(); // close the input file
		
        buffer = new SFZAudioBuffer(myChannels, length);
        int position = 0;
        
        
        if(myBitsPerSample == 16)
        {
            short int *temp;

            temp = (short int *)&myData[0];
            
            // FIXME: endian swap?
            
                for(int i=0; i<length; i++)
                    for(int j=0; j<myChannels; j++)
                        buffer->channels[j][i] = (float)temp[position++] / 32768.0f;
        }
        else if(myBitsPerSample == 24)
        {
            // FIXME: endian swap?
            
            for(int i=0; i<length; i++)
                for(int j=0; j<myChannels; j++)
                {
                    
                    int32_t sample = (unsigned char)myData[position+2];
                    sample = (sample << 8) | (unsigned char)myData[position+1];
                    sample = (sample << 8) | (unsigned char)myData[position];
                    sample <<= 8;
                    
                    buffer->channels[j][i] = (float)((double)sample / (double)INT32_MAX);
                    
                    position += 3;
                }
        }
        else if(myBitsPerSample == 32)
        {
            // FIXME: endian swap?
            
            int32_t *temp;
            
            temp = (int32_t *)&myData[0];
            
            for(int i=0; i<length; i++)
                for(int j=0; j<myChannels; j++)
                    buffer->channels[j][i] = (float)((double)temp[position++] / (double)INT32_MAX);
        }
        
        
        delete myData;
        myData = 0;
		
	}else {
        //		cout << "ERROR: Could not load sample: " <<myPath << endl; //This line seems to be hated by windows
        printf("ERROR: Could not load sample.");
        result = false;
	}
	
	
	return result; // this should probably be something more descriptive
}


int32_t SFZAudioReader::parseSMPLChunk(ifstream &f, long dataLength)
{
    // FIXME: endian swap?
    
	f.read((char *)&samplerChunk, sizeof(TSamplerChunk));
	
	if (samplerChunk.cSampleLoops)
	{
        // just read the first one?
		f.read((char *)&sampleLoop, sizeof(TSampleLoop));
	}
    
    
    
	return 1;
}



long SFZAudioReader::getLength() {
    return length;
}



void SFZAudioReader::clear() {
    memset(myData, 0, myDataSize);
}

void SFZAudioReader::reset() {
    position=0;
}
