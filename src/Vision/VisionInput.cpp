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
	if ( profile.isCamera() )
	{
		stopFile();
		
		return setupWithCamera(profile);
	}
	else
	{
		stopCamera();
		
		return setupWithFile(profile);
	}
}

void VisionInput::stop()
{
	stopCamera();
	stopFile();
}

void VisionInput::stopCamera()
{
	if (mCapture) mCapture->stop();
	mCapture.reset();
}

void VisionInput::stopFile()
{
	mDebugFrame.reset();
	mDebugFrameFileWatch.clear();
}

void VisionInput::waitForFrame( chrono::nanoseconds debugFrameSleep )
{
	if ( isCamera() )
	{
//		mCapture->waitForNewFrame();
		this_thread::sleep_for(2.5ms); // @30fps 1frame = 33ms, @60fps 1frame = 16ms
			// 5ms seems too high to reach 30fps, but 2.5 seems to work
			// ideally we could block inside of mInput.getFrame()
			
			// TODO: rationalize all this waiting by keeping track of desired FPS--for file and camera--
			// and running execution time to predict how long to wait. debugFrameSleep		
	}
	else this_thread::sleep_for(debugFrameSleep);
}

SurfaceRef VisionInput::getFrame()
{
	SurfaceRef frame;
	
	// camera?
	if ( mCapture && mCapture->checkNewFrame() )
	{
		frame = mCapture->getSurface();
	}
	
	// file?
	if ( mDebugFrame )
	{
		mDebugFrameFileWatch.update();
		frame = mDebugFrame;
	}
	
	// return
	return frame;
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