//
//  Instrument.hpp
//  PaperBounce3
//
//  Created by Luke Iannini on 11/14/16.
//
//

#ifndef Instrument_hpp
#define Instrument_hpp

#include <stdio.h>
#include "GameWorld.h"
#include "FileWatch.h"
#include "PureDataNode.h"
#include "RtMidi.h"

typedef vector<int> Scale;


// meta-params
enum class MetaParam
{
	Scale=0,
	RootNote,
	Tempo,
	//		BeatCount,
	kNumMetaParams
};

class MetaParamInfo
{
public:
	bool isDiscrete() const { return mNumDiscreteStates!=-1; }

	int mNumDiscreteStates=-1;
};

typedef std::shared_ptr<RtMidiOut> RtMidiOutRef;


// instrument info
class Instrument
{
public:

	~Instrument();

	void setParams( XmlTree );
	void setup();

	int channelForNote(int note);

	// colors!
	ColorA mPlayheadColor;
	ColorA mScoreColor;
	ColorA mNoteOffColor, mNoteOnColor;

	//
	string mName;

	//
	enum class SynthType
	{
		Additive = 1,
		MIDI	 = 2,
		Meta	 = 3  // controls global params
	};

	SynthType mSynthType;
	MetaParam mMetaParam; // only matters if mSynthType==Meta



	// midi
	int  mPort=0;
	int  mChannel=0;
	int  mMapNotesToChannels=0; // for KORG Volca Sample
	RtMidiOutRef mMidiOut;


	struct tOnNoteInfo
	{
		float mStartTime;
		float mDuration;
	};

	map < int , tOnNoteInfo > mOnNotes ;
	// (instrument,note) -> (start time, duration)

	bool isNoteInFlight( int note ) const;
	void updateNoteOffs();
	void doNoteOn( int note, float duration ); // start time is now
	void doNoteOff( int note );

	void killAllNotes();
private:
	// midi convenience methods
	void sendMidi( RtMidiOutRef, uchar, uchar, uchar );
	void sendNoteOn ( RtMidiOutRef midiOut, uchar channel, uchar note, uchar velocity );
	void sendNoteOff ( RtMidiOutRef midiOut, uchar channel, uchar note );



};

typedef std::shared_ptr<Instrument> InstrumentRef;


#endif /* Instrument_hpp */
