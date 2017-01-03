//
//  FileWatch.cpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 8/8/16.
//
//

#include "FileWatch.h"

void
FileWatch::update()
{
	time_t now = time(nullptr);
	
	double elapsed = difftime(now,mLastScan);
	
	if ( elapsed > mScanSecInterval )
	{
		scanFiles();
		mLastScan = now ;
	}
}

void FileWatch::scanFiles()
{
	for( auto &i : mFileWatch )
	{
		if ( boost::filesystem::exists(i.first) )
		{
			time_t lastreadtime = i.second.first ;
			time_t modtime = boost::filesystem::last_write_time(i.first);
			
			if ( modtime > lastreadtime )
			{
				doLoad( i.first, i.second.second );
				
				i.second.first = modtime;
			}
		}
	}
}

void FileWatch::load( fs::path path, tCallback func )
{
//	cout << "FileWatch::load " << path << endl ;
	
	doLoad(path,func);

	time_t modtime = boost::filesystem::exists(path)
						? boost::filesystem::last_write_time(path)
						: (time_t)0; // epoch; not found, so keep trying...

	mFileWatch[path] = pair<time_t,tCallback>(modtime,func) ;
}

void FileWatch::load ( vector<fs::path> ps, tMultiCallback func )
{
	auto cb = [func]( fs::path )
	{
		if (func) func();
	};
	
	for( auto p : ps )
	{
		load( p, cb );
	}
}

void FileWatch::loadXml( fs::path path, tXmlCallback func )
{
	load( path, [func](fs::path path)
	{
		try
		{
			XmlTree xml( DataSourcePath::create(path) );
			func(xml) ;
		}
		catch(cinder::Exception e)
		{
			cout << "loadXml, failed to load: " << e.what() << endl;
		}
		catch( ... )
		{
			cout << "loadXml, failed to load " << path << endl ;
		}
	});
}

void FileWatch::loadShader( fs::path vert, fs::path frag, tGlslProgCallback func )
{
	std::vector<fs::path> paths = {
		vert, frag
	};
	
	load( paths, [vert,frag,func]()
	{
		gl::GlslProgRef shader;
		
		try {
			shader = gl::GlslProg::create( DataSourcePath::create(vert), DataSourcePath::create(frag) );
		}
		catch(cinder::Exception e) { cout << "GlslProg exception: " << e.what() << endl; }

		if (!shader)
		{
			cout << "FileWatch::loadShader: failed to load '" << vert << "', '" << frag << "'" << endl;
		}
		
		func(shader);
	});
	
//	load( vert, [loadShader](fs::path){loadShader();} );
//	load( frag, [loadShader](fs::path){loadShader();} );
	
	// this will load the shader 2x on first load.
	// whatever. if this was a problem, then we could manually update last read time for both files in the lambda.
}

void FileWatch::doLoad( fs::path path, tCallback func )
{
	cout << "FileWatch::doLoad " << path << endl;
	
	if ( boost::filesystem::exists(path) )
	{
		func(path);
	}
}
