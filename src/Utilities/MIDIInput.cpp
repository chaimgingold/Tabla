//
//  MIDIInput.cpp
//  Tabla
//
//  Created by Chaim Gingold on 2/6/17.
//
//

#include "MIDIInput.h"

using namespace std;

MIDIInput::MIDIInput()
{
}

void MIDIInput::openAllPorts()
{
	mPorts.clear();
	
	try
	{
		unsigned int nPorts=0;
		
		// port scan
		{
			auto scanner = RtMidiInRef( new RtMidiIn() );

			nPorts = scanner->getPortCount();
		
			if (mVerbose) {
				cout << "MIDIInput: " << nPorts << " ports" << endl;
				for( int i=0; i<nPorts; ++i ) cout << "\t" << i << ": " << scanner->getPortName(i) << endl;
			}
		}
		
		// open all
		for( int i=0; i<nPorts; ++i )
		{
			Port p;
			p.mIndex = mPorts.size();
			p.mIn = RtMidiInRef( new RtMidiIn() );
			p.mName = p.mIn->getPortName(i);
			
			if (mVerbose) cout << "MIDIInput opening port " << i << ": " << p.mName << endl;

			p.mIn->openPort(i);
			
			if (p.mIn->isPortOpen())
			{
				mPorts.push_back(p);
			}
			else cout << "Failed to open MIDI port " << p.mName << endl;
		}
		
	} catch ( RtMidiError e ) {
		cout << e.getMessage() << endl;
	}
}

void MIDIInput::read()
{
	try
	{
		for( auto &port : mPorts )
		{
			double stamp;
			std::vector<unsigned char> msg;
			
			do
			{
				stamp = port.mIn->getMessage(&msg);
				
				if (!msg.empty())
				{
					int nBytes = msg.size();
					
					// log it
					if (mVerbose)
					{
						cout << "MIDI Input Packet from " << port.mName << endl;
						cout << "\tStamp = " << stamp << endl;
						
						for ( int i=0; i<nBytes; i++ ) {
							std::cout << "\tByte " << i << " = " << (int)msg[i] << endl; //", ";
						}
					}

					// control change
					if ( nBytes==3 && (msg[0] & 0xF0) == 176 ) // 0b10110000
					{
						decodeControlChange(port,msg);
					}
				}
			}
			while( !msg.empty() );
		}
	}
	catch ( RtMidiError e ) {
		cout << e.getMessage() << endl;
	}
}

void MIDIInput::decodeControlChange( Port& port, tMsg msg )
{
	// decode
	int channel = (msg[0] & 0x0F);
	int control =  msg[1];
	int value   =  msg[2];
	
	float valueNorm = (float)msg[2] / (float)127;
	
	// update value
	bool isNew = port.mControls.find(control)==port.mControls.end();
	
	port.mControls[control].mIntValue = value;
	port.mControls[control].mFloatValue = valueNorm;
	
	// discover
	if (isNew) {
		cout << "NEW: " << port.mIndex << ", " << control << endl;
		discoveredNewControl(port.mIndex,control);
	}
	
	// callback
	{
		auto bind = mControlToUserName.find( PortControlIndex(port.mIndex,control) );
		
		if ( bind != mControlToUserName.end() )
		{
			auto lambda = mControlChangeLambdas.find(bind->second);
			
			if (lambda != mControlChangeLambdas.end() && lambda->second)
			{
				lambda->second(port.mControls[control]);
			}
		}
	}
	
	// log
	if (mVerbose)
	{
		cout << "MIDI[\"" << port.mName << "\"]"
			 << ".channel[" << channel << "].control[" << control << "] = "
			 << value << " (" << std::fixed << std::setprecision(3) << valueNorm << ")"
			 << endl;
	}
}

void MIDIInput::discoverControls( vector<string> names )
{
	for ( auto n : names ) mDiscoverControls.push(n);
}

void MIDIInput::discoveredNewControl( int port, int control )
{
	if ( !mDiscoverControls.empty() )
	{
		string name = mDiscoverControls.front();
		mDiscoverControls.pop();
		
		mControlToUserName[ PortControlIndex(port,control) ] = name;
		mUserNameToControl[ name ] = PortControlIndex(port,control);
		
		if (mVerbose) cout << "Discovered control '" << name << "'" << endl;
	}
}