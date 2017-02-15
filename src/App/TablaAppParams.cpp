//
//  TablaAppParams.cpp
//  Tabla
//
//  Created by Chaim Gingold on 2/15/17.
//
//

#include "TablaAppParams.h"
#include "xml.h"

void TablaAppParams::set( XmlTree xml )
{
	getXml(xml,"AutoFullScreenProjector",mAutoFullScreenProjector);
	getXml(xml,"DrawCameraImage",mDrawCameraImage);
	getXml(xml,"DrawContours",mDrawContours);
	getXml(xml,"DrawContoursFilled",mDrawContoursFilled);
	getXml(xml,"DrawMouseDebugInfo",mDrawMouseDebugInfo);
	getXml(xml,"DrawPolyBoundingRect",mDrawPolyBoundingRect);
	getXml(xml,"DrawContourTree",mDrawContourTree);
	getXml(xml,"DrawPipeline",mDrawPipeline);
	getXml(xml,"DrawContourMousePick",mDrawContourMousePick);
	
	getXml(xml,"ConfigWindowMainImageDrawBkgndImage",mConfigWindowMainImageDrawBkgndImage);
	
	getXml(xml,"HasConfigWindow",mHasConfigWindow);
	getXml(xml,"ConfigWindowPipelineWidth",mConfigWindowPipelineWidth);
	getXml(xml,"ConfigWindowPipelineGutter",mConfigWindowPipelineGutter);
	getXml(xml,"ConfigWindowMainImageMargin",mConfigWindowMainImageMargin);
	
	getXml(xml,"KeyboardStringTimeout",mKeyboardStringTimeout);
	
	getXml(xml,"DefaultPixelsPerWorldUnit",mDefaultPixelsPerWorldUnit);
	getXml(xml,"DebugFrameSkip",mDebugFrameSkip);
	
	getXml(xml,"DefaultGameWorld",mDefaultGameWorld);	
}
