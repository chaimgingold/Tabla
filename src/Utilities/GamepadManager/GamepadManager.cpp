//
//  GamepadManager.cpp
//  PaperBounce3
//
//  Created by Chaim Gingold on 12/6/16.
//
//

#include "GamepadManager.h"
#include <iostream>

using namespace std;

static bool verbose = false;

static void onButtonDown(struct Gamepad_device * device, unsigned int buttonID, double timestamp, void * context)
{
	static_cast<GamepadManager*>(context)->onButtonDown(device, buttonID, timestamp);
	
    if (verbose) 
    {
        cout << "Button " << buttonID << " down on device " << device->deviceID << " at " << timestamp << " with context " << context << std::endl;
    }
}

static void onButtonUp(struct Gamepad_device * device, unsigned int buttonID, double timestamp, void * context)
{
	static_cast<GamepadManager*>(context)->onButtonUp(device, buttonID, timestamp);

    if (verbose)
    {
        cout << "Button " << buttonID << " up on device " << device->deviceID << " at " << timestamp << " with context " << context << std::endl;
    }
}

static void onAxisMoved(struct Gamepad_device * device, unsigned int axisID, float value, float lastValue, double timestamp, void * context)
{
	static_cast<GamepadManager*>(context)->onAxisMoved(device,axisID,value,lastValue,timestamp);

	const float kDeadZone = .2f;
	
    if (verbose && (value < -kDeadZone || value > kDeadZone)) // reduce the output noise by making a dead zone
    {
        cout << "Axis " << axisID << " moved from " << lastValue <<" to " << value << " on device " << device->deviceID << " at " << timestamp << " with context " << context << std::endl;
    }
}

static void onDeviceAttached(struct Gamepad_device * device, void * context)
{
	static_cast<GamepadManager*>(context)->onDeviceAttached(device);

    if (verbose)
    {
        cout << "Device ID " << device->deviceID << " attached (vendor = " << device->vendorID <<"; product = " << device->productID << ") with context" << context << std::endl;
    }
}

static void onDeviceRemoved(struct Gamepad_device * device, void * context)
{
	static_cast<GamepadManager*>(context)->onDeviceRemoved(device);

    if (verbose)
    {
        cout << "Device ID " << device->deviceID << " removed with context " << context << std::endl;
    }
}

const GamepadManager::Device*
GamepadManager::getDeviceById( DeviceId id ) const
{
	for( GamepadManager::Device* d : mDevices )
	{
		if ( d->deviceID == id ) return d;
	}
	
	return 0;
}

GamepadManager::GamepadManager()
{
	void* context = this;
	
    Gamepad_deviceAttachFunc(::onDeviceAttached, context );
    Gamepad_deviceRemoveFunc(::onDeviceRemoved, context);
    Gamepad_buttonDownFunc(::onButtonDown, context);
    Gamepad_buttonUpFunc(::onButtonUp, context);
    Gamepad_axisMoveFunc(::onAxisMoved, context);
    Gamepad_init();
	
	mIterationsToNextPoll=mDetectNewDevicesPollFreq;
}

GamepadManager::~GamepadManager()
{
    Gamepad_shutdown();
}

void GamepadManager::tick()
{
    mIterationsToNextPoll--;
    if (mIterationsToNextPoll == 0) {
        Gamepad_detectDevices();
        mIterationsToNextPoll = mDetectNewDevicesPollFreq;
    }

	//
    Gamepad_processEvents();
}

void GamepadManager::onButtonDown		( Gamepad_device * device, unsigned int buttonID, double timestamp )
{
	if (mOnButtonDown) mOnButtonDown( Event(device,EventType::ButtonDown,buttonID,timestamp) );
}

void GamepadManager::onButtonUp			( Gamepad_device * device, unsigned int buttonID, double timestamp )
{
	if (mOnButtonUp) mOnButtonUp( Event(device,EventType::ButtonUp,buttonID,timestamp) );
}

void GamepadManager::onAxisMoved		( Gamepad_device * device, unsigned int axisID, float value, float lastValue, double timestamp )
{
	if (mOnAxisMoved) mOnAxisMoved( Event(device,axisID,value,lastValue,timestamp) );
}

void GamepadManager::onDeviceAttached	( Gamepad_device * device )
{
	mDevices.push_back(device);
	
	if (mOnDeviceAttached) mOnDeviceAttached( Event(device,EventType::DeviceAttached) );
}

void GamepadManager::onDeviceRemoved	( Gamepad_device * device )
{
	mDevices.erase(
		std::remove_if(
			mDevices.begin(), 
			mDevices.end(),
			[device](Device*d){return d==device;}),
		mDevices.end());
	
	if (mOnDeviceRemoved) mOnDeviceRemoved( Event(device,EventType::DeviceRemoved) );
}