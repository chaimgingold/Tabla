//
//  Instrument.cpp
//  PaperBounce3
//
//  Created by Luke Iannini on 11/14/16.
//
//
#include "PaperBounce3App.h" // for hotloadable file paths
#include "geom.h"
#include "cinder/rand.h"
#include "cinder/audio/Context.h"
#include "cinder/audio/Source.h"
#include "xml.h"
#include "Pipeline.h"
#include "ocv.h"
#include "RtMidi.h"


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
	getXml(xml,"NoteOffColor",mNoteOffColor);
	getXml(xml,"NoteOnColor",mNoteOnColor);

	getXml(xml,"Name",mName);

	getXml(xml,"IconFileName",mIconFileName);
	
	if ( xml.hasChild("SynthType") )
	{
		string t = xml.getChild("SynthType").getValue();

		map<string,SynthType> synths;
		synths["Additive"] = SynthType::Additive;
		synths["MIDI"] = SynthType::MIDI;
		synths["RobitPokie"] = SynthType::RobitPokie;
		synths["Meta"] = SynthType::Meta;
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
void midiErrorCallback(RtMidiError::Type type, const std::string &errorText, void *userData) {
	cout << "MIDI out error: " << type << errorText << endl;
}

void Instrument::setup()
{
	assert(!mMidiOut);

	switch (mSynthType) {
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
	return mSynthType==Instrument::SynthType::MIDI || mSynthType==Instrument::SynthType::RobitPokie;
}

void Instrument::setupMIDI() {
	cout << "Opening port " << mPort << " for '" << mName << "'" << endl;

	// RtMidiOut can throw an OSError if it has trouble initializing CoreMIDI.
	try {
		mMidiOut = make_shared<RtMidiOut>();
	} catch (std::exception) {
		cout << "Error creating RtMidiOut " << mPort << " for '" << mName << "'" << endl;
	}

	if (mMidiOut) {

		mMidiOut->setErrorCallback(midiErrorCallback, NULL);
		if (mPort < mMidiOut->getPortCount()) {
			mMidiOut->openPort( mPort );
		} else {
			cout << "...Opening virtual port for '" << mName << "'" << endl;
			mMidiOut->openVirtualPort(mName);
		}
	}
}

void Instrument::setupSerial() {
	// iterate through ports
	auto ports = SerialPort::getPorts();
	for (auto port : ports) {
		cout << "SERIAL DEVICE" << endl;
		cout << "\tNAME: " << port->getName() << endl;
		cout << "\tDESCRIPTION: " << port->getDescription() << endl;
		cout << "\tHARDWARE IDENTIFIER: " << port->getHardwareIdentifier() << endl;
	}

	// grab a port and create a device
	if (!ports.empty()) {
		SerialPortRef port = SerialPort::findPortByNameMatching(std::regex("\\/dev\\/cu\\.usbserial.*"));
		if (!port) {
			port = ports.back();
		}

		try {
			mSerialDevice = SerialDevice::create(port, 115200);
		} catch (serial::IOException& e) {
			cout << "Failed to create serial device : ( " << e.what() << endl;
		}

		// NB: device is opened on creation
	}
}

// For most synths this is just the assigned channel,
// but in the case of the Volca Sample we want to
// send each note to a different channel as per its MIDI implementation.
int Instrument::channelForNote(int note) const {
	if (mMapNotesToChannels > 0) {
		return note % mMapNotesToChannels;
	}
	return mChannel;
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
		if (mSynthType == SynthType::RobitPokie) {
			bool off = now > it.second.mStartTime + mPokieRobitPulseTime;
			if (off) {
				// stash noteInfo since it gets cleared in doNoteOff,
				// send a "fake" super-short note
				// and then restore it to make sure we wait til "real" note is over before retriggering
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

			default:
				break;
		}


		tOnNoteInfo i;
		i.mStartTime = ci::app::getElapsedSeconds();
		i.mDuration  = duration;

		mOnNotes[ note ] = i;
	}
}

void Instrument::sendSerialByte(const uint8_t charByte, const uint8_t hiLowByte) {
	if (!mSerialDevice) {
		return;
	}

	// outgoing message is 5 bytes
	const size_t bufSize = 3;
	uint8_t buffer[bufSize] = {
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
	if (arpeggiator.Mode != ArpMode::None) {

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

		default:
			break;
	}

	mOnNotes.erase(note);
}

// We use 'A' to denote Pokie #1, 'B' to denote Pokie #2, etc.
uint8_t Instrument::serialCharForNote(int note) {
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

void Instrument::killAllNotes() {
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
