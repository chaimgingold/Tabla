//
//  GetCaptureDeviceFormats.h
//  Tabla
//
//  Created by Chaim Gingold on 2/25/17.
//
//

#ifndef GetCaptureDeviceFormats_h
#define GetCaptureDeviceFormats_h

#include <vector>
#include "cinder/Vector.h"

struct CaptureDeviceFormat
{
	ci::ivec2 mSize;
	float mMaxFPS;
};

std::vector<CaptureDeviceFormat> getCaptureDeviceFormats( void* nativeDevice );

#endif /* GetCaptureDeviceResolutions_h */
