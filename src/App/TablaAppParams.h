//
//  TablaAppParams.hpp
//  Tabla
//
//  Created by Chaim Gingold on 2/15/17.
//
//

#ifndef TablaAppParams_hpp
#define TablaAppParams_hpp

#include "cinder/Xml.h"

using namespace ci;
using namespace std;

class TablaAppParams
{
public:
	void set( XmlTree );
	
	bool mAutoFullScreenProjector = false ;
	bool mDrawCameraImage = false ;
	bool mDrawContours = false ;
	bool mDrawContoursFilled = false ;
	bool mDrawMouseDebugInfo = false ;
	bool mDrawPolyBoundingRect = false ;
	bool mDrawContourTree = false ;
	bool mDrawPipeline = false;
	bool mDrawContourMousePick = false;
	bool mConfigWindowMainImageDrawBkgndImage = true;
	
	bool  mHasConfigWindow = true;
	float mConfigWindowMainImageMargin = 32.f;
	float mConfigWindowPipelineGutter = 8.f ;
	float mConfigWindowPipelineWidth  = 64.f ;
	
	float mDefaultPixelsPerWorldUnit = 10.f; // doesn't quite hot-load; you need to 
	
	int mDebugFrameSkip = 30;

	float mKeyboardStringTimeout=.2f; // how long to wait before clearing the keyboard string buffer?
	string mDefaultGameWorld = "BallWorld";
};

#endif /* TablaAppParams_hpp */
