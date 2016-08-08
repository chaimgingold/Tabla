//
//  XmlFileWatch.h
//  PaperBounce3
//
//  Created by Chaim Gingold on 8/8/16.
//
//

#ifndef XmlFileWatch_hpp
#define XmlFileWatch_hpp

#include "cinder/Xml.h"
#include "boost/filesystem/path.hpp"
#include <ctime>

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace boost;

class XmlFileWatch
{
public:
	typedef function<void(XmlTree)> tXmlCallback;

	void	update(); // periodically does file scan
	void	scanFiles(); // does a file scan
	void	load( fs::path, tXmlCallback ); // ask for a file
	
	float	mScanSecInterval = 1.f ;
	
private:

	time_t mLastScan=0;
	
	time_t doLoad( fs::path, tXmlCallback );
	
	map< fs::path, pair<time_t,tXmlCallback> > mXmlFileWatch ;

	
};

#endif /* XmlFileWatch_hpp */
