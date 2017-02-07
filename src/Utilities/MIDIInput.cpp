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
	mVerbose=true;
}

void MIDIInput::openAllPorts()
{
	mIn.clear();
	
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
			RtMidiInRef in( new RtMidiIn() );
			
			if (mVerbose) cout << "MIDIInput opening port " << i << ": " << in->getPortName(i) << endl;

			in->openPort(i);
			
			if (in->isPortOpen())
			{
				mIn.push_back(in);
			}
			
			mIn.push_back(in);
		}
		
	} catch ( RtMidiError e ) {
		cout << e.getMessage() << endl;
	}
}

void MIDIInput::read()
{
	try
	{
		for( auto in : mIn )
		{
			double stamp;
			std::vector<unsigned char> msg;
			
			do
			{
				stamp = in->getMessage(&msg);
				
				if (!msg.empty())
				{
					int nBytes = msg.size();
					
					if (mVerbose && nBytes>0)
					{
						cout << "MIDI Input Packet" << endl;
						cout << "\tStamp = " << stamp << endl;
						
		//				cout << "\t";
						for ( int i=0; i<nBytes; i++ ) {
							std::cout << "\tByte " << i << " = " << (int)msg[i] << endl; //", ";
						}
		//				cout << endl;
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
