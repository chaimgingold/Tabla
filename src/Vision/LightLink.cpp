//
//  LightLink.cpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 8/5/16.
//
//

#include "LightLink.h"
#include "xml.h"

static string noNewline( string str )
{
	for( auto &c : str ) {
		if (c=='\n'||c=='\r') c=' ';
	}
	return str;
}

static vector<float> stringToFloatVec( string value )
{
	// clobber [], chars
	for( int i=0; i<value.size(); ++i )
	{
		if (value[i]==','||value[i]=='['||value[i]==']'||value[i]==';')
		{
			value[i]=' ';
		}
	}		
	
	// break into float vector
	vector<float> f;
	
	std::istringstream ss(value);
	while ( !ss.fail() )
	{
		float x;
		ss >> x;
		if (!ss.fail()) f.push_back(x);
	}
	
	return f;
}

LightLink::CaptureProfile::CaptureProfile( string name, vec2 size, float pixelsPerWorldUnit )
	: mName(name)
	, mCaptureSize(size)
{
	setCaptureCoords(mCaptureSize);
	setWorldCoordsFromCaptureCoords(pixelsPerWorldUnit);
}

LightLink::CaptureProfile::CaptureProfile( string name, string deviceName, vec2 size, float pixelsPerWorldUnit )
	: CaptureProfile(name,size,pixelsPerWorldUnit)
{
	mDeviceName = deviceName;
}

LightLink::CaptureProfile::CaptureProfile( fs::path path, vec2 size, float pixelsPerWorldUnit )
	: CaptureProfile(path.filename().string(),size,pixelsPerWorldUnit)
{
	mFilePath = path.string();
}

void LightLink::CaptureProfile::setCaptureCoords( vec2 size )
{
	mCaptureCoords[0] = vec2(0,0) * size;
	mCaptureCoords[1] = vec2(1,0) * size;
	mCaptureCoords[2] = vec2(1,1) * size;
	mCaptureCoords[3] = vec2(0,1) * size;
}

void LightLink::CaptureProfile::setWorldCoordsFromCaptureCoords( float pixelsPerWorldUnit )
{
	for( int i=0; i<4; ++i )
	{
		mCaptureWorldSpaceCoords[i] = mCaptureCoords[i] / pixelsPerWorldUnit ;
	}
}

void LightLink::CaptureProfile::flipCaptureCoordsVertically()
{
	swap( mCaptureCoords[0], mCaptureCoords[3] );
	swap( mCaptureCoords[1], mCaptureCoords[2] );
}

void LightLink::CaptureProfile::flipCaptureCoordsHorizontally()
{
	swap( mCaptureCoords[0], mCaptureCoords[1] );
	swap( mCaptureCoords[2], mCaptureCoords[3] );
}

void LightLink::CaptureProfile::setParams( XmlTree xml )
{
	const bool kVerbose = false;
	
	getXml(xml, "Name",mName);
	getXml(xml, "DeviceName",mDeviceName);
	getXml(xml, "FileName",mFilePath);
	getXml(xml, "CaptureSize",mCaptureSize);

	if ( !getVec2sFromXml(xml, "CaptureCoords", mCaptureCoords, 4 ) ) {
		// backwards compatibility
		getXml(xml, "CaptureCoords", mCaptureCoords, 4 );
	}
	
	if ( !getVec2sFromXml(xml, "CaptureWorldSpaceCoords", mCaptureWorldSpaceCoords, 4 ) ) {
		// backwards compatibility
		getXml(xml, "CaptureWorldSpaceCoords", mCaptureWorldSpaceCoords, 4 );
	}

	if ( xml.hasChild("DistCoeffs") )
	{
		vector<float> f = stringToFloatVec( xml.getChild("DistCoeffs").getValue() );
		
		if (f.size() > 0)
		{
			mDistCoeffs = cv::Mat(1,f.size(),CV_32F);
			for( int i=0; i<f.size(); ++i ) mDistCoeffs.at<float>(i) = f[i];
			if (kVerbose) cout << "DistCoeffs: " << mDistCoeffs << endl;
		}
	}

	if ( xml.hasChild("CameraMatrix") )
	{
		vector<float> f = stringToFloatVec( xml.getChild("CameraMatrix").getValue() );
		
		if (f.size() == 9)
		{
			mCameraMatrix = cv::Mat(3,3,CV_32F);
			for( int i=0; i<f.size(); ++i ) mCameraMatrix.at<float>(i) = f[i];
			if (kVerbose) cout << "CameraMatrix: " << mCameraMatrix << endl;
		}
	}
}

XmlTree LightLink::CaptureProfile::getParams() const
{
	XmlTree t("Profile","");
	
	t.push_back( XmlTree( "Name", mName) );
	t.push_back( XmlTree( "DeviceName", mDeviceName) );
	t.push_back( XmlTree( "FileName", mFilePath) );
	t.push_back( XmlTree( "CaptureSize", vecToString(mCaptureSize) ) );
	t.push_back( vec2sToXml( "CaptureCoords", mCaptureCoords,4 ) ) ;
	t.push_back( vec2sToXml( "CaptureWorldSpaceCoords", mCaptureWorldSpaceCoords, 4 ) );

	if ( mDistCoeffs.rows==1 )
	{
		std::ostringstream ss;
		ss << mDistCoeffs;
		t.push_back( XmlTree( "DistCoeffs", noNewline(ss.str()) ) );
	}

	if ( !mCameraMatrix.empty() )
	{
		std::ostringstream ss;
		ss << mCameraMatrix;
		t.push_back( XmlTree( "CameraMatrix", noNewline(ss.str()) ) );
	}
	
	return t;
}

LightLink::ProjectorProfile::ProjectorProfile( string name, vec2 size, const vec2 captureWorldSpaceCoords[4] )
	: mName(name)
	, mProjectorSize(size)
{
	mProjectorCoords[0] = vec2(0,0) * size;
	mProjectorCoords[1] = vec2(1,0) * size;
	mProjectorCoords[2] = vec2(1,1) * size;
	mProjectorCoords[3] = vec2(0,1) * size;
	
	if (captureWorldSpaceCoords)
	{
		for( int i=0; i<4; ++i ) {
			mProjectorWorldSpaceCoords[i] = captureWorldSpaceCoords[i];
		}
	}
	else
	{
		for( int i=0; i<4; ++i ) {
			mProjectorWorldSpaceCoords[i] = mProjectorCoords[i] / 10.f ; // 10px per cm as a default...
		}
	}
}

void LightLink::ProjectorProfile::setParams( XmlTree xml )
{
	getXml(xml, "Name",mName);

	if ( !getVec2sFromXml(xml, "ProjectorWorldSpaceCoords", mProjectorWorldSpaceCoords, 4 ) ) {
		// backwards compatibility
		getXml(xml, "ProjectorWorldSpaceCoords", mProjectorWorldSpaceCoords, 4 );
	}

	getXml(xml, "ProjectorSize", mProjectorSize);
	
	if ( !getVec2sFromXml(xml, "ProjectorCoords", mProjectorCoords, 4 ) ) {
		// backwards compatibility
		getXml(xml, "ProjectorCoords", mProjectorCoords, 4 );
	}	
}

XmlTree LightLink::ProjectorProfile::getParams() const
{
	XmlTree t("Profile","");
	
	t.push_back( XmlTree( "Name", mName) );
	t.push_back( vec2sToXml( "ProjectorWorldSpaceCoords", mProjectorWorldSpaceCoords,4) );
	t.push_back( XmlTree( "ProjectorSize", vecToString(mProjectorSize)) );
	t.push_back( vec2sToXml( "ProjectorCoords", mProjectorCoords,4) );
	
	return t;
}

void LightLink::setParams( XmlTree xml )
{
	getXml(xml,"CaptureProfile",mActiveCaptureProfileName);
	getXml(xml,"ProjectorProfile",mActiveProjectorProfileName);
	
	mCaptureProfiles.clear();
	for( auto i = xml.begin( "Capture/Profile" ); i != xml.end(); ++i )
	{
		CaptureProfile p;
		p.setParams(*i);
		mCaptureProfiles[p.mName] = p;
	}
	
	mProjectorProfiles.clear();
	for( auto i = xml.begin( "Projector/Profile" ); i != xml.end(); ++i )
	{
		ProjectorProfile p;
		p.setParams(*i);
		mProjectorProfiles[p.mName] = p;
	}
	
	ensureActiveProfilesAreValid();
}

XmlTree LightLink::getParams() const
{
	XmlTree t("LightLink","");
	
	t.push_back( XmlTree("","  Active Profiles  ",0,XmlTree::NODE_COMMENT) );

	t.push_back( XmlTree("CaptureProfile", mActiveCaptureProfileName) );
	t.push_back( XmlTree("ProjectorProfile", mActiveProjectorProfileName) );
	
	t.push_back( XmlTree("","  Capture Profiles  ",0,XmlTree::NODE_COMMENT) );
	
	XmlTree capture("Capture","");
	for( const auto& p : mCaptureProfiles ) {
		capture.push_back( p.second.getParams() );
	}
	t.push_back(capture);

	t.push_back( XmlTree("","  Projector Profiles  ",0,XmlTree::NODE_COMMENT) );

	XmlTree projector("Projector","");
	for( const auto& p : mProjectorProfiles ) {
		projector.push_back( p.second.getParams() );
	}
	t.push_back(projector);
	
	return t;
}

LightLink::CaptureProfile&
LightLink::getCaptureProfile()
{
	auto i = mCaptureProfiles.find(mActiveCaptureProfileName);
	assert( i != mCaptureProfiles.end() );
	return i->second;
}

LightLink::ProjectorProfile&
LightLink::getProjectorProfile()
{
	auto i = mProjectorProfiles.find(mActiveProjectorProfileName);
	assert( i != mProjectorProfiles.end() );
	return i->second;
}

const LightLink::CaptureProfile&
LightLink::getCaptureProfile() const
{
	auto i = mCaptureProfiles.find(mActiveCaptureProfileName);
	assert( i != mCaptureProfiles.end() );
	return i->second;
}

const LightLink::ProjectorProfile&
LightLink::getProjectorProfile() const
{
	auto i = mProjectorProfiles.find(mActiveProjectorProfileName);
	assert( i != mProjectorProfiles.end() );
	return i->second;
}

vector<const LightLink::CaptureProfile*>
LightLink::getCaptureProfilesForDevice( string deviceName ) const
{
	vector<const CaptureProfile*> result;
	
	for( const auto &p : mCaptureProfiles )
	{
		if (p.second.mDeviceName==deviceName)
		{
			result.push_back( &(p.second) );
		}
	}
	
	return result;
}

LightLink::CaptureProfile*
LightLink::getCaptureProfileForFile( string filePath )
{
	for( auto &p : mCaptureProfiles )
	{
		if (p.second.mFilePath==filePath)
		{
			return &p.second;
		}
	}
	
	return 0;	
}

bool LightLink::ensureActiveProfilesAreValid()
{
//	assert( !mProjectorProfiles.empty() );
//	assert( !mCaptureProfiles.empty() );
//  don't do this anymore
	bool dirty=false;
	
	if ( !mCaptureProfiles.empty() && mCaptureProfiles.find(mActiveCaptureProfileName) == mCaptureProfiles.end() )
	{
		mActiveCaptureProfileName = mCaptureProfiles.begin()->second.mName;
		dirty=true;
	}

	if ( !mProjectorProfiles.empty() && mProjectorProfiles.find(mActiveProjectorProfileName) == mProjectorProfiles.end() )
	{
		mActiveProjectorProfileName = mProjectorProfiles.begin()->second.mName;
		dirty=true;
	}
	
	return dirty;
}

void LightLink::eraseCaptureProfile( string name )
{
	auto i = mCaptureProfiles.find(name);
	if (i!=mCaptureProfiles.end())
	{
		mCaptureProfiles.erase(i);
		ensureActiveProfilesAreValid();	
	}
}

void LightLink::setCaptureProfile  ( string name ) {
	mActiveCaptureProfileName=name;
	assert(mCaptureProfiles.find(name)!=mCaptureProfiles.end());
}

void LightLink::setProjectorProfile( string name ) {
	mActiveProjectorProfileName=name;
	assert(mProjectorProfiles.find(name)!=mProjectorProfiles.end());
}
