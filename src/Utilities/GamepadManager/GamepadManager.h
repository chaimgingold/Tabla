//
//  GamepadManager.hpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 12/6/16.
//
//

#ifndef GamepadManager_hpp
#define GamepadManager_hpp

#include "Gamepad.h"

class GamepadManager
{
public:
	GamepadManager();
	~GamepadManager();
	
	void tick();
	
	int mDetectNewDevicesPollFreq = 30;
	
	class Event
	{
	public:
		// button up/down
		Event( Gamepad_device* device, unsigned int buttonid, double timestamp )
		: mDevice(device)
		, mId(buttonid)
		, mTimestamp(timestamp) {}

		// axis move
		Event( Gamepad_device* device, unsigned int axisid, float value, float lastvalue,  double timestamp )
		: mDevice(device)
		, mId(axisid)
		, mAxisValue(value)
		, mLastAxisValue(lastvalue)
		, mTimestamp(timestamp) {}
		
		// device move
		Event( Gamepad_device* device )
		: mDevice(device) {}
		
		Gamepad_device *mDevice=0;
		
		unsigned int mId=0; // button or axis id
		float  mAxisValue=0.f, mLastAxisValue=0.f; // only for axis events
		double mTimestamp=0.f; // none for device attached/removed
	};

	// devices
	typedef Gamepad_device Device;
	typedef unsigned int DeviceId;
	typedef std::vector<Device*> DeviceVec;
	const DeviceVec& getDevices() const { return mDevices; }	

	// your callbacks
	typedef std::function<void( const Event& )> tEventLambda;

	tEventLambda mOnButtonDown;
	tEventLambda mOnButtonUp;
	tEventLambda mOnAxisMoved;
	tEventLambda mOnDeviceAttached;
	tEventLambda mOnDeviceRemoved;

	void clearCallbacks()
	{
		mOnButtonDown = 0;
		mOnButtonUp = 0;
		mOnAxisMoved = 0;
		mOnDeviceAttached = 0;
		mOnDeviceRemoved = 0;
	}
		
	// internal callbacks -- callbacks from driver into us.
	// (should be private)
	void onButtonDown		( Gamepad_device * device, unsigned int buttonID, double timestamp );
	void onButtonUp			( Gamepad_device * device, unsigned int buttonID, double timestamp );
	void onAxisMoved		( Gamepad_device * device, unsigned int axisID, float value, float lastValue, double timestamp );
	void onDeviceAttached	( Gamepad_device * device );
	void onDeviceRemoved	( Gamepad_device * device );

private:
	DeviceVec mDevices;
	
	unsigned int mIterationsToNextPoll;

};

#endif /* GamepadManager_hpp */
