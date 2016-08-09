//
//  XmlFileWatch.cpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 8/8/16.
//
//

#include "XmlFileWatch.h"

void
XmlFileWatch::update()
{
	time_t now = time(nullptr);
	
	double elapsed = difftime(now,mLastScan);
	
	if ( elapsed > mScanSecInterval )
	{
		scanFiles();
		mLastScan = now ;
	}
}

void XmlFileWatch::scanFiles()
{
	for( auto i : mXmlFileWatch )
	{
		if ( boost::filesystem::exists(i.first) )
		{
			time_t modtime = i.second.first ;
			
			if ( modtime < boost::filesystem::last_write_time(i.first) )
			{
				i.second.first = doLoad( i.first, i.second.second );
			}
		}
	}
}

void XmlFileWatch::load( fs::path path, tXmlCallback func )
{
	cout << "loadXml " << path << endl ;
	
	time_t modtime = doLoad(path,func);

	mXmlFileWatch[path] = pair<time_t,tXmlCallback>(modtime,func) ;
}

time_t XmlFileWatch::doLoad( fs::path path, tXmlCallback func )
{
	try
	{
		XmlTree xml( DataSourcePath::create(path) );
		
		func(xml) ;

		if ( boost::filesystem::exists(path) )
		{
			return boost::filesystem::last_write_time(path);
		}
	}
	catch( ... )
	{
		cout << "loadXml, failed to load " << path << endl ;
	}

	return (time_t)0 ; // epoch
}
