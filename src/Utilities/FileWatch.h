//
//  FileWatch.h
//  PaperBounce3
//
//  Created by Chaim Gingold on 8/8/16.
//
//

#ifndef FileWatch_hpp
#define FileWatch_hpp

#include "cinder/Xml.h"
#include "boost/filesystem/path.hpp"
#include <ctime>

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace boost;

class FileWatch
{
public:
	typedef function<void(fs::path)> tCallback;
	typedef function<void(void)> tMultiCallback;
	typedef function<void(XmlTree)> tXmlCallback;
	typedef function<void(gl::GlslProgRef)> tGlslProgCallback;

	void	clear() { *this = FileWatch(); }
	void	update(); // periodically does file scan
	void	scanFiles(); // does a file scan
	
	// ask to load files
	void	load		( fs::path, tCallback );
	void	load		( vector<fs::path>, tMultiCallback ); // load as a group; WIP, but providing proper api
	
	void	loadXml		( fs::path, tXmlCallback ); // get an XmlTree
	void	loadShader	( fs::path vert, fs::path frag, tGlslProgCallback ); // get a glsl prog; might be 0 if parse fail
	
	float	mScanSecInterval = 1.f ;
	
private:

	time_t mLastScan=0;
	
	void doLoad( fs::path, tCallback );
	
	map< fs::path, pair<time_t,tCallback> > mFileWatch ;

	
};

#endif /* FileWatch_hpp */
