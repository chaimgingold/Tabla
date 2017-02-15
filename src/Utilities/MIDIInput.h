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
#include <queue>

using namespace std;

class MIDIInput
{
public:
	MIDIInput();
	
	void openAllPorts();
	void read();

	class Control
	{
	public:
		int   mIntValue=64;
		float mFloatValue=.5f;
	};

	typedef std::function<void( const Control& )> tControlChangeLambda;

	void discoverControls( vector<string> userNames );
	void setControlLambda( string name, tControlChangeLambda f ) { mControlChangeLambdas[name]=f; }
	
	void setVerbose( bool v=true ) { mVerbose=v; }
	bool isVerbose() const { return mVerbose; }
	
private:
	bool mVerbose=false;

	typedef std::shared_ptr<RtMidiIn> RtMidiInRef;
	
	class Port
	{
	public:
		int mIndex=-1;
		RtMidiInRef mIn;
		string mName;
		map<int,Control> mControls;
	};
	
	vector<Port> mPorts;
	queue<string> mDiscoverControls; // names to assign to newly discovered controls
	
	typedef std::vector<unsigned char> tMsg;
	
	void decodeControlChange( Port&, tMsg );
	void discoveredNewControl( int port, int control );
	
	typedef pair<int,int> PortControlIndex;
	map< PortControlIndex, string > mControlToUserName;
	map< string, PortControlIndex > mUserNameToControl;
	map< string, tControlChangeLambda > mControlChangeLambdas;
};

#endif /* MIDIInput_hpp */