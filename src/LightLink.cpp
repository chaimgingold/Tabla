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
	
	getXml(xml, "CaptureCoords", mCaptureCoords, 4 );
	getXml(xml, "CaptureWorldSpaceCoords", mCaptureWorldSpaceCoords, 4 );
	
//	for ( int i=0; i<4; ++i ) cout << mCaptureCoords[i] << endl;
//	for ( int i=0; i<4; ++i ) cout << mCaptureWorldSpaceCoords[i] << endl;
}

