//
//  LightLink.h
//  PaperBounce3
//
//  Created by Chaim Gingold on 8/5/16.
//
//

#ifndef LightLink_h
#define LightLink_h

#include "cinder/Xml.h"
#include "CinderOpenCV.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class LightLink
{
public:

	// calibration for camera <> world <> projector
	void setParams( XmlTree );
	XmlTree getParams() const;
	
	class CaptureProfile
	{
	public:
		CaptureProfile() {}
		CaptureProfile( string name, string deviceName, vec2 size ); // makes a default dummy
		
		void setParams( XmlTree );
		XmlTree getParams() const;
		
		string mName; // user name
		string mDeviceName; // references system device to use

		//  • Capture
		vec2 mCaptureSize = vec2(640,480) ;
		
		//  • Clip/Deskew
		vec2 mCaptureCoords[4];
		
		//  • Map to World Space
		vec2 mCaptureWorldSpaceCoords[4];

		// • Barrel distortion correct
		cv::Mat mDistCoeffs;
		cv::Mat mCameraMatrix;
	};
	
	class ProjectorProfile
	{
	public:
		ProjectorProfile() {}
		ProjectorProfile( string name, vec2 size, vec2 captureWorldSpaceCoords[4]=0 );
			// make a default
			// last param is optional, but allows this default projector to use same world space as current camera
		
		void setParams( XmlTree );
		XmlTree getParams() const;
		
		string mName;
		
		// • Map from World Space
		vec2 mProjectorWorldSpaceCoords[4];
		
		// • Map into Camera Space
		vec2 mProjectorSize = vec2(1280,800) ;
		vec2 mProjectorCoords[4];
	};
	
	// active profiles
	CaptureProfile& getCaptureProfile();
	ProjectorProfile& getProjectorProfile();

	const CaptureProfile& getCaptureProfile() const;
	const ProjectorProfile& getProjectorProfile() const;

	// all of them
	map<string,CaptureProfile> mCaptureProfiles;
	map<string,ProjectorProfile> mProjectorProfiles;

	void setCaptureProfile  ( string name );
	void setProjectorProfile( string name );

	void ensureActiveProfilesAreValid();
		// ensures active profile names point to valid things
		// (if there are no profiles, then it does nothing.) 
		// setParams() calls this function after loading from xml on its own.

private:
	string mActiveCaptureProfileName;
	string mActiveProjectorProfileName;

};

#endif /* LightLink_h */
