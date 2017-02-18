//
//  VisionInput.cpp
//  Tabla
//
//  Created by Chaim Gingold on 2/17/17.
//
//

#include "VisionInput.h"


bool VisionInput::setup( const LightLink::CaptureProfile& profile )
{
	mDebugFrame.reset();
	mDebugFrameFileWatch.clear();
	
	if ( profile.isCamera() ) {
		return setupWithCamera(profile);
	} else {
		return setupWithFile(profile);
	}
}

void VisionInput::stop()
{
	if (mCapture) mCapture->stop();
	
	mDebugFrame.reset();
	mDebugFrameFileWatch.clear();
}

SurfaceRef VisionInput::getFrame()
{
	if (   (mCapture && mCapture->checkNewFrame())
		|| (mDebugFrame && (mDebugFrameSkip<2 || getElapsedFrames()%mDebugFrameSkip==0)) )
	{
		// get image
		if (mDebugFrame) {
			mDebugFrameFileWatch.update();
			return mDebugFrame;
		} else {
			return mCapture->getSurface();
		}
	}
	return SurfaceRef();
}

bool VisionInput::setupWithFile( const LightLink::CaptureProfile& profile )
{
	cout << "Trying to load file capture profile '" << profile.mName << "' for file '" << profile.mFilePath << "'" << endl;
	
	mDebugFrameFileWatch.clear();
	
	mDebugFrameFileWatch.load( profile.mFilePath, [this,profile]( fs::path )
	{
		try {
			mDebugFrame = make_shared<Surface>( loadImage(profile.mFilePath) );
		} catch (...){
			mDebugFrame.reset();
		}
	});
	
	return mDebugFrame != nullptr;
	// ideally we should erase it if it fails to load, but that complicates situations where
	// caller has an iterator into capture profiles (eg setupNextValidCaptureProfile). this isn't
	// that hard, but might not be that important.
	// we also might want to always return true so that user can escape missing files (removing from list)
}

bool VisionInput::setupWithCamera ( const LightLink::CaptureProfile& profile )
{
	cout << "Trying to load camera capture profile '" << profile.mName << "' for '" << profile.mDeviceName << "'" << endl;
	
	Capture::DeviceRef device = Capture::findDeviceByNameContains(profile.mDeviceName);

	if ( device )
	{
		if ( !mCapture || mCapture->getDevice() != device
			|| mCapture->getWidth () != (int32_t)profile.mCaptureSize.x
			|| mCapture->getHeight() != (int32_t)profile.mCaptureSize.y
			)
		{
			if (mCapture) {
				// kill old one
				mCapture->stop();
				mCapture = 0;
			}
			
			try
			{
				mCapture = Capture::create(profile.mCaptureSize.x, profile.mCaptureSize.y,device);
				mCapture->start();
				return true;
			} catch (...) {
				cout << "Failed to init capture device " << profile.mDeviceName << endl;
				return false;
			}
		}
		return true; // lazily true
	}
	else return false;
}