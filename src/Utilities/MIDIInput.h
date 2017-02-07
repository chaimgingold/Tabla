//
//  MIDIInput.hpp
//  Tabla
//
//  Created by Chaim Gingold on 2/6/17.
//
//

#ifndef MIDIInput_hpp
#define MIDIInput_hpp

#include "RtMidi.h"
#include <memory>

using namespace std;

class MIDIInput
{
public:
	MIDIInput();
	
	void openAllPorts();
	void read();

	void setVerbose( bool v ) { mVerbose=v; }
	
private:
	bool mVerbose=false;
	
	typedef std::shared_ptr<RtMidiIn> RtMidiInRef;
	
	vector<RtMidiInRef> mIn;
	
};

#endif /* MIDIInput_hpp */