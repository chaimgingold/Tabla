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
	
	enum class EventType
	{
		ButtonDown,
		ButtonUp,
		AxisMoved,
		DeviceAttached,
		DeviceRemoved
	};
	
	class Event
	{
	public:
		// button up/down
		Event( Gamepad_device* device, EventType type, unsigned int buttonid, double timestamp )
		: mDevice(device)
		, mType(type)
		, mId(buttonid)
		, mTimestamp(timestamp) {}

		// axis moved
		Event( Gamepad_device* device, unsigned int axisid, float value, float lastvalue,  double timestamp )
		: mDevice(device)
		, mType(EventType::AxisMoved)
		, mId(axisid)
		, mAxisValue(value)
		, mLastAxisValue(lastvalue)
		, mTimestamp(timestamp) {}
		
		// device attached/removed
		Event( Gamepad_device* device, EventType type )
		: mDevice(device)
		, mType(type) {}
		
		Gamepad_device *mDevice=0;
		
		EventType mType;
		unsigned int mId=0; // button or axis id
		float  mAxisValue=0.f, mLastAxisValue=0.f; // only for axis events
		double mTimestamp=0.f; // none for device attached/removed
	};

	// devices
	typedef Gamepad_device Device;
	typedef unsigned int DeviceId;
	typedef std::vector<Device*> DeviceVec;
	const DeviceVec& getDevices() const { return mDevices; }	
	const Device* getDeviceById( DeviceId ) const;
	
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
