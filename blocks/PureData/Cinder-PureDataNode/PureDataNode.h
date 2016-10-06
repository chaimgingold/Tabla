#pragma once

#include "PdBase.hpp"
#include "PdReceiver.hpp"
#include "PdTypes.hpp"

#include "cinder/audio/Node.h"
#include "cinder/Thread.h"

namespace cipd {

class PureDataPrintReceiver : public pd::PdReceiver {

public:
	void print(const std::string& message) override;
};


typedef std::shared_ptr<pd::Patch> PatchRef;
typedef std::shared_ptr<class PureDataNode> PureDataNodeRef;

class PureDataNode : public ci::audio::Node {
public:
	PureDataNode( const Format &format = Format() );
	~PureDataNode();

	void initialize() override;
	void uninitialize() override;
	void process( ci::audio::Buffer *buffer ) override;

	pd::PdBase& getPd()	{ return mPdBase; }

	void		addToPath( cinder::fs::path path );

	PatchRef	loadPatch( ci::DataSourceRef dataSource );
	void		closePatch( const PatchRef &patch );

	// thread-safe senders
	void sendBang( const std::string& dest );
	void sendFloat( const std::string& dest, float value );
	void sendSymbol( const std::string& dest, const std::string& symbol );
	void sendList( const std::string& dest, const pd::List& list );
	void sendMessage( const std::string& dest, const std::string& msg, const pd::List& list = pd::List() );

	bool readArray( const std::string& arrayName, std::vector<float>& dest, int readLen = -1, int offset = 0 );
	bool writeArray( const std::string& arrayName, std::vector<float>& source, int writeLen = -1, int offset = 0 );
	void clearArray( const std::string& arrayName, int value = 0 );

private:
	pd::PdBase	mPdBase;
	std::mutex	mMutex;
	size_t		mNumTicksPerBlock;

	ci::audio::BufferInterleaved mBufferInterleaved;

	PureDataPrintReceiver mPdReceiver;
};


} // namespace cipd
