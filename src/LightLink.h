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

using namespace ci;
using namespace ci::app;
using namespace std;

class LightLink
{
public:

	// calibration for camera <> world <> projector
	void setParams( XmlTree );
	
	vec2 getCaptureSize() const { return mCaptureSize; }
	
	
//private:
	
	// Pipeline looks like this:
	
	// -- Camera --
	
	//  • Capture
	vec2 mCaptureSize = vec2(640,480) ;
	
	//  • Fix barrel distortion
	//  float mUnbarrelDistort ;
	
	
	//  • Clip/Deskew
	vec2 mCaptureCoords[4];
	
	//  • Map to World Space
	vec2 mCaptureWorldSpaceCoords[4];
	
	// -- Machine vision + Simulation happens --
	
	
	// -- Projector --
	
	// • Map from World Space
	// vec2 mProjectorWorldSpaceCoords[4];
	
	// • Map into Camera Space
	// vec2 mProjectorCoords[4];
	
	// • Barrel distortion correct
	
};

#endif /* LightLink_h */
