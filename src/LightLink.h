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
	
	
private:
	
	vec2 mCaptureSize = vec2(640,480) ;
	
	
};

#endif /* LightLink_h */
