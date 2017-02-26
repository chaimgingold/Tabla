//
//  GetCaptureDeviceResolutions.mm
//  Tabla
//
//  Created by Chaim Gingold on 2/25/17.
//
//

#include "GetCaptureDeviceFormats.h"
#import <AVFoundation/AVCaptureDevice.h>

using namespace cinder;
using namespace std;

#if defined( CINDER_MAC ) || defined( CINDER_COCOA_TOUCH_DEVICE )

const bool verbose = true;

vector<CaptureDeviceFormat> getCaptureDeviceFormats( void* nativeDevice ) 
{
	AVCaptureDevice* device = (__bridge AVCaptureDevice*)nativeDevice;
	
	vector<CaptureDeviceFormat> out;
	
	if (device)
	{
		for ( AVCaptureDeviceFormat *format in [device formats] )
		{
			CMFormatDescriptionRef formatDescription = format.formatDescription;
			
			CaptureDeviceFormat outf;
			
			{
				float maxrate=((AVFrameRateRange*)[format.videoSupportedFrameRateRanges objectAtIndex:0]).maxFrameRate;
				
				outf.mMaxFPS = maxrate;
				
				if (verbose) cout << "fps " << maxrate << endl;
			}
			
			if( formatDescription )
			{
				CMVideoDimensions dimensions = ::CMVideoFormatDescriptionGetDimensions( formatDescription );
				
				outf.mSize = ivec2(dimensions.width,dimensions.height);
				
				if (verbose) cout << "res " << dimensions.width << "x" << dimensions.height << endl;
			}

			out.push_back(outf);
		}
	}
	
	return out;
}

#else

vector<ivec2> getCaptureDeviceResolutions( void* nativeDevice )
{
	// some dummy,
	// but this should really go in another file
	return vector<ivec2> sizes = { vec2(1920,1080), vec2(640,480) };
}

#endif