//
//  Instrument.cpp
//  PaperBounce3
//
//  Created by Luke Iannini on 11/14/16.
//
//
#include "TablaApp.h" // for hotloadable file paths
#include "geom.h"
#include "cinder/rand.h"
#include "cinder/audio/Context.h"
#include "cinder/audio/Source.h"
#include "xml.h"
#include "Pipeline.h"
#include "ocv.h"
#include "RtMidi.h"
#include "Score.h"

using namespace std::chrono;
using namespace ci::gl;

#include "Instrument.h"

float MetaParamInfo::discretize( float v ) const
{
	v = roundf( (float)mNumDiscreteStates * v );
	v = min( v, (float)mNumDiscreteStates-1 );
	v /= (float)mNumDiscreteStates;
	return v;
}

void Instrument::setParams( XmlTree xml )
{
	getXml(xml,"PlayheadColor",mPlayheadColor);
	getXml(xml,"ScoreColor",mScoreColor);
	getXml(xml,"ScoreColorDownLines",mScoreColorDownLines);
	getXml(xml,"NoteOffColor",mNoteOffColor);
	getXml(xml,"NoteOnColor",mNoteOnColor);
	
	getXml(xml,"Name",mName);

	getXml(xml,"IconFileName",mIconFileName);
	getXml(xml,"IconGradientCenter",mIconGradientCenter);

	if ( xml.hasChild("SynthType") )
	{
		string t = xml.getChild("SynthType").getValue();

		map<string,SynthType> synths;
		synths["Additive"] = SynthType::Additive;
		synths["MIDI"] = SynthType::MIDI;
		synths["RobitPokie"] = SynthType::RobitPokie;
		synths["Meta"] = SynthType::Meta;
		synths["Sampler"] = SynthType::Sampler;
		auto i = synths.find(t);
		mSynthType = (i!=synths.end()) ? i->second : SynthType::MIDI ; // default to MIDI

		if ( mSynthType==SynthType::Meta )
		{
			string p = xml.getChild("MetaParam").getValue();

			map<string,MetaParam> params;
			params["Scale"] = MetaParam::Scale;
			params["RootNote"] = MetaParam::RootNote;
			params["Tempo"] = MetaParam::Tempo;
			auto i = params.find(p);
			mMetaParam = (i!=params.end()) ? i->second : MetaParam::RootNote ; // default to RootNote
		}
	}

	getXml(xml,"Port",mPort);
	getXml(xml,"Channel",mChannel);
	getXml(xml,"MapNotesToChannels",mMapNotesToChannels);
}

Instrument::~Instrument()
{
	if (mMidiOut) mMidiOut->closePort();
}

// Setting this keeps RtMidi from throwing exceptions when something goes wrong talking to the MIDI system.
void midiErrorCallback(RtMidiError::Type type, const std::string &errorText, void *userData)
{
	cout << "MIDI out error: " << type << errorText << endl;
}

void Instrument::setup()
{
	assert(!mMidiOut);

	switch (mSynthType)
	{
        case SynthType::MIDI:
			setupMIDI();
			break;
		case SynthType::RobitPokie:
			setupSerial();
			break;
		default:
			break;
	}
}

bool Instrument::isNoteType() const
{
	return mSynthType==Instrument::SynthType::MIDI
		|| mSynthType==Instrument::SynthType::RobitPokie
		|| mSynthType==Instrument::SynthType::Sampler;
}

bool Instrument::isAvailable() const
{
	switch( mSynthType )
	{
        case SynthType::MIDI:
			return mMidiOut.get();
			break;
		case SynthType::RobitPokie:
			return mSerialDevice.get();
			break;
		default:
			return true;
			break;
	}
}

void Instrument::setupMIDI()
{
	cout << "Opening port " << mPort << " for '" << mName << "'" << endl;

	// RtMidiOut can throw an OSError if it has trouble initializing CoreMIDI.
	try
	{
		mMidiOut = make_shared<RtMidiOut>();
	}
	catch (std::exception)
	{
		cout << "Error creating RtMidiOut " << mPort << " for '" << mName << "'" << endl;
	}

	if (mMidiOut)
	{
		mMidiOut->setErrorCallback(midiErrorCallback, NULL);
		if (mPort < mMidiOut->getPortCount())
		{
			mMidiOut->openPort( mPort );
		}
		else
		{
			cout << "...Opening virtual port for '" << mName << "'" << endl;
			mMidiOut->openVirtualPort(mName);
		}
	}
}

void Instrument::setupSerial()
{
	// iterate through ports
	auto ports = SerialPort::getPorts();
	for (auto port : ports) {
		cout << "SERIAL DEVICE" << endl;
		cout << "\tNAME: " << port->getName() << endl;
		cout << "\tDESCRIPTION: " << port->getDescription() << endl;
		cout << "\tHARDWARE IDENTIFIER: " << port->getHardwareIdentifier() << endl;
	}

	// grab a port and create a device
	if (!ports.empty())
	{
		SerialPortRef port = SerialPort::findPortByNameMatching(std::regex("\\/dev\\/cu\\.usbserial.*"));
		if (!port)
		{
			port = ports.back();
		}

		try
		{
			mSerialDevice = SerialDevice::create(port, 115200);
		}
		catch (serial::IOException& e)
		{
			cout << "Failed to create serial device : ( " << e.what() << endl;
		}

		// NB: device is opened on creation
	}
}

// For most synths this is just the assigned channel,
// but in the case of the Volca Sample we want to
// send each note to a different channel as per its MIDI implementation.
int Instrument::channelForNote(int note) const {
	if (mMapNotesToChannels > 0)
	{
		return note % mMapNotesToChannels;
	}
	return mChannel;
}

int Instrument::noteForY( const Score& score, int y ) const
{
	if (mMapNotesToChannels && mSynthType != Instrument::SynthType::RobitPokie ) {
		// Reinterpret octave shift as note shift when using NotesToChannelsMode (and don't go negative)
		int noteShift = score.mOctave + score.mNumOctaves/2;
		return y + noteShift;
	}

	int numNotes = score.mScale.size();
	int extraOctaveShift = y / numNotes * 12;
	int degree = y % numNotes;
	int note = score.mScale[degree];

	return note + extraOctaveShift + score.mRootNote + score.mOctave*12;
}

bool Instrument::isNoteInFlight( int note ) const
{
	auto i = mOnNotes.find( note );
	return i != mOnNotes.end();
}

void Instrument::updateNoteOffs()
{
	const float now = ci::app::getElapsedSeconds();

	// search for "expired" notes and send them their note-off MIDI message,
	// then remove them from the mOnNotes map

	auto currentNotes = mOnNotes; // copy so we can mutate while iterating

	for ( auto it : currentNotes )
	{
		bool off = now > it.second.mStartTime + it.second.mDuration;

		int note = it.first;

		// HACK: this is a hack to test super-short pulses
		if (mSynthType == SynthType::RobitPokie)
		{
			bool off = now > it.second.mStartTime + mPokieRobitPulseTime;
			if (off)
			{
				// stash noteInfo since it gets cleared in doNoteOff,
				// send a "fake" super-short note
				// and then restore it to make sure we continue
				// waiting til "real" note
				// is over before retriggering
				auto noteInfo = mOnNotes[note];
				doNoteOff(note);
				mOnNotes[note] = noteInfo;
			}
		}

		if (off)
		{
			doNoteOff(note);
		}
	}
}

void Instrument::doNoteOn( int note, float duration )
{
	if ( !isNoteInFlight(note) )
	{
		uchar velocity = 100; // 0-127

		uchar channel = channelForNote(note);


		switch (mSynthType)
		{
			case SynthType::RobitPokie:
				sendSerialByte(serialCharForNote(note), '1');
				break;

			case SynthType::MIDI:
				sendNoteOn( mMidiOut, channel, note, velocity );
				break;
			case SynthType::Sampler:
				mOpenSFZ->mSynth->noteOn(1, note, (float)velocity/127.0);
				break;
			default:
				break;
		}


		tOnNoteInfo i;
		i.mStartTime = ci::app::getElapsedSeconds();
		i.mDuration  = duration;

		mOnNotes[ note ] = i;
	}
}

void Instrument::sendSerialByte(const uint8_t charByte, const uint8_t hiLowByte)
{
	if (!mSerialDevice)
	{
		return;
	}

	// outgoing message is 5 bytes
	const size_t bufSize = 3;
	uint8_t buffer[bufSize] =
	{
		charByte,
		hiLowByte,
		'Q' // Q == message terminator
	};

	size_t size = mSerialDevice->writeBytes(buffer, bufSize);

	if (size != bufSize) {
		printf("only sent %i bytes, should have sent %i!\n", (int)size, (int)bufSize);
		return;
	}
}

void Instrument::tickArpeggiator()
{
	if (mArpeggiator.Mode != ArpMode::None) {

	}
}

void Instrument::doNoteOff( int note )
{
	switch (mSynthType)
	{
		case SynthType::RobitPokie:
			sendSerialByte(serialCharForNote(note), '0');
			break;

		case SynthType::MIDI:
			sendNoteOff( mMidiOut, channelForNote(note), note);
			break;
		case SynthType::Sampler:
			mOpenSFZ->mSynth->noteOff(1, note, true);
		default:
			break;
	}

	mOnNotes.erase(note);
}

// We use 'A' to denote Pokie #1, 'B' to denote Pokie #2, etc.
uint8_t Instrument::serialCharForNote(int note)
{
	return 'A' + note % mMapNotesToChannels;
}

void Instrument::sendNoteOn ( RtMidiOutRef midiOut, uchar channel, uchar note, uchar velocity )
{
	const uchar noteOnBits = 9;

	uchar channelBits = channel & 0xF;

	sendMidi( midiOut, (noteOnBits<<4) | channelBits, note, velocity);
}

void Instrument::sendNoteOff ( RtMidiOutRef midiOut, uchar channel, uchar note )
{
	const uchar velocity = 0; // MIDI supports "note off velocity", but that's esoteric and we're not using it
	const uchar noteOffBits = 8;

	uchar channelBits = channel & 0xF;

	sendMidi( midiOut, (noteOffBits<<4) | channelBits, note, velocity);
}

void Instrument::sendMidi( RtMidiOutRef midiOut, uchar a, uchar b, uchar c )
{
	std::vector<uchar> message(3);

	message[0] = a;
	message[1] = b;
	message[2] = c;

	if (midiOut) midiOut->sendMessage( &message );

}

void Instrument::killAllNotes()
{
	if (mMidiOut)
	{
		for (int channel = 0; channel < 16; channel++)
		{
			for (int note = 0; note < 128; note++)
			{
				sendNoteOff( mMidiOut, channel, note );
			}
		}
	}
}

void Instrument::updateSynthesis( const vector<Score const*>& scores )
{
	switch (mSynthType)
	{
		// Additive
		case Instrument::SynthType::Additive:
			updateAdditiveSynthesis(scores);
			break;

		// Notes
		case Instrument::SynthType::Sampler:
		case Instrument::SynthType::MIDI:
		case Instrument::SynthType::RobitPokie:
			updateNoteSynthesis(scores);
			break;
		
		default:
		break;
	}	
}

void Instrument::updateSynthesisWithVision( const Scores& scores )
{
	switch (mSynthType)
	{
		// Additive
		case Instrument::SynthType::Additive:
			updateAdditiveSynthesisWithVision(scores);
			break;
			
		default:
		break;
	}
}

void Instrument::updateNoteSynthesis( const Scores& scores )
{
	set<NoteNum> onSet; // notes that are presently on in our scores 
	
	for( auto score : scores )
	{
		// send midi notes
		if (!score->mNotes.empty())
		{
			int x = score->getPlayheadFrac() * (float)score->mNotes.mNumCols;

			for ( int y=0; y<score->mNotes.size(); ++y )
			{
				int note = noteForY(*score,y);
				
				const ScoreNote* isOn = score->mNotes.isNoteOn(x,y);
				
				if (isOn)
				{
					float duration =
						score->mBeatDuration *
						(float)score->getQuantizedBeatCount() *
						isOn->mLengthAsScoreFrac;
						// Note: that if we trigger late, we will go on for too long...

					if (duration>0)
					{
						doNoteOn( note, duration );
					}

					onSet.insert(note);
				}
			}
		}
	}
	
	// turn off in-flight notes that aren't on right now
	vector<NoteNum> turnOff;
	
	for( auto n : mOnNotes )
	{
		if ( onSet.find(n.first)==onSet.end() )
		{
			turnOff.push_back(n.first); // queue them up so we don't mess up iterator
		}
	}

	for( auto n : turnOff ) doNoteOff(n); 

	// retire notes (do this outside of scores to handle instruments that no longer have scores)	
	updateNoteOffs();
	
	// arpeggiate
	tickArpeggiator();
}

void Instrument::updateAdditiveSynthesis( const Scores& scores ) const
{
	// Update time (for first score)
	if ( !scores.empty() )
	{
		mPd->sendFloat(toString(mAdditiveSynthID)+string("phase"),
					   scores[0]->getPlayheadFrac() );
	}			
}
		
void Instrument::updateAdditiveSynthesisWithVision( const Scores& scores ) const
{
	// Mute all additive synths, in case their score has disappeared (FIXME: do this in ~Score() ?)
	{
		const int kMaxSynths = 8; // This corresponds to [clone 8 music-voice] in music.pd

		for( int additiveSynthID=0; additiveSynthID<kMaxSynths; ++additiveSynthID )
		{
			mPd->sendFloat(toString(additiveSynthID)+string("volume"), 0);
		}
	}

	// grab 1st score-- and ignore the others
	if (scores.empty()) return;
	const Score& score = *(scores[0]);
	
	// send image for additive synthesis
	if ( !score.mImage.empty() )
	{
		PureDataNodeRef pd = mPd;
		int additiveSynthID = mAdditiveSynthID;
		
		int rows = score.mImage.rows;
		int cols = score.mImage.cols;

		string prefix = toString(additiveSynthID);
		
		// Update pan
		pd->sendFloat(prefix+"pan",
					  score.mPan);

		// Update per-score pitch
		pd->sendFloat(prefix+"note-root",
					  20 );

		// Update range of notes covered by additive synthesis
		pd->sendFloat(prefix+"note-range",
					  100 );

		// Update resolution
		pd->sendFloat(prefix+"resolution-x",
					  cols);

		pd->sendFloat(prefix+"resolution-y",
					  rows);

		// Create a float version of the image
		cv::Mat imageFloatMat;

		// Copy the uchar version scaled 0-1
		score.mImage.convertTo(imageFloatMat, CV_32FC1, 1/255.0);
		// TODO: optimize-- do convertTo after slicing out column  

		// Convert to a vector to pass to Pd

		// Grab the current column at the playhead. We send updates even if the
		// phase doesn't change enough to change the colIndex,
		// as the pixels might have changed instead.
		float phase = score.getPlayheadFrac();
		int colIndex = imageFloatMat.cols * phase;

		cv::Mat columnMat = imageFloatMat.col(colIndex);
		
		// We use a list message rather than writeArray as it causes less contention with Pd's execution thread
		auto ampsByFreq = pd::List();
		for (int i = 0; i < columnMat.rows; i++) {
			ampsByFreq.addFloat(columnMat.at<float>(i, 0));
		}

		pd->sendList(prefix+"vals", ampsByFreq);

		// Turn the synth on
		pd->sendFloat(prefix+"volume", 1);
	}
}

InstrumentRef InstrumentRefs::hasSynthType( Instrument::SynthType t ) const
{
	for ( auto i : *this ) {
		if (i->mSynthType==t) return i;
	}
	return 0;
}

bool InstrumentRefs::hasInstrument( InstrumentRef i ) const
{
	for ( auto j : *this ) {
		if (j==i) return true;
	}
	return false;	
}

InstrumentRef InstrumentRefs::hasNoteType() const
{
	for ( auto i : *this ) {
		if (i->isNoteType()) return i;
	}
	return 0;
}
