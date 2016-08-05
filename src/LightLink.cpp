//
//  LightLink.cpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 8/5/16.
//
//

#include "LightLink.h"
#include "xml.h"

void LightLink::setParams( XmlTree xml )
{
	getXml(xml,"CaptureSize",mCaptureSize);
}

