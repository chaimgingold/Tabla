//
//  SerialDevice.h
//  Cinder-Serial
//
//  Created by Jean-Pierre Mouilleseaux on 29 Jun 2015.
//  Copyright 2015 Chorded Constructions. All rights reserved.
//

#pragma once

#include "cinder/Cinder.h"
#if defined(CINDER_MSW)
    #pragma comment(lib, "setupapi.lib")
#endif

#include <string>
#include <vector>
#include <regex>

#include "serial.h"

namespace Cinder { namespace Serial {

typedef std::shared_ptr<class SerialPort> SerialPortRef;
typedef std::shared_ptr<class SerialDevice> SerialDeviceRef;

typedef serial::Timeout Timeout;

static const Timeout TimeoutDefault = serial::Timeout(serial::Timeout::max(), 0, 0, 1000, 0);

enum class DataBits {
    Unknown = -1,
    Five = serial::fivebits,
    Six = serial::sixbits,
    Seven = serial::sevenbits,
    Eight = serial::eightbits,
};

enum class Parity {
    Unknown = -1,
    None = serial::parity_none,
    Odd = serial::parity_odd,
    Even = serial::parity_even,
    Mark = serial::parity_mark,
    Space = serial::parity_space,
};

enum class StopBits {
    Unknown = -1,
    One = serial::stopbits_one,
    Two = serial::stopbits_two,
    OnePointFive = serial::stopbits_one_point_five,
};

enum class FlowControl {
    Unknown = -1,
    None = serial::flowcontrol_none,
    Hardware = serial::flowcontrol_hardware,
    Software = serial::flowcontrol_software,
};

class SerialPort : public std::enable_shared_from_this<SerialPort> {
public:
    static const std::vector<SerialPortRef>& getPorts(bool forceRefresh = false);
    static SerialPortRef findPortByNameMatching(const std::regex& pattern, bool forceRefresh = false);

    ~SerialPort() = default;

    const std::string getName() const;
    const std::string getDescription() const;
    const std::string getHardwareIdentifier() const;

private:
    static SerialPortRef create(const serial::PortInfo& info) { return SerialPortRef(new SerialPort(info))->shared_from_this(); }
    SerialPort(const serial::PortInfo& info) : mInfo(info) {};

    static std::vector<SerialPortRef> sPorts;
    static bool sPortsDirty;

    serial::PortInfo mInfo;
};

class SerialDevice : public std::enable_shared_from_this<SerialDevice> {
public:
    static SerialDeviceRef create(const SerialPortRef port, uint32_t baudRate, const Timeout& timeout = TimeoutDefault, DataBits dataBits = DataBits::Eight, Parity parity = Parity::None, StopBits stopBits = StopBits::One, FlowControl flowControl = FlowControl::None);
    static SerialDeviceRef create(const std::string& portName, uint32_t baudRate, const Timeout& timeout = TimeoutDefault, DataBits dataBits = DataBits::Eight, Parity parity = Parity::None, StopBits stopBits = StopBits::One, FlowControl flowControl = FlowControl::None);
    ~SerialDevice();

    const std::string getPortName() const;
    uint32_t getBaudRate() const;
    const Timeout getTimeout() const;
    DataBits getDataBits() const;
    Parity getParity() const;
    StopBits getStopBits() const;
    FlowControl getFlowControl() const;

    bool isOpen() const;
    void open();
    void close();

    size_t getNumBytesAvailable() const;
    size_t readBytes(uint8_t* buffer, size_t maxSize);
    size_t writeBytes(const uint8_t* buffer, size_t size);

    void flush();
    void flushInput();
    void flushOutput();

private:
    typedef std::shared_ptr<class serial::Serial> SerialRef;

    SerialDevice(const std::string& portName, uint32_t baudRate, const Timeout& timeout, DataBits dataBits, Parity parity, StopBits stopBits, FlowControl flowControl);

    SerialRef mSerial;
};

}}
