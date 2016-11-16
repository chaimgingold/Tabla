//
//  SerialDevice.cpp
//  Cinder-Serial
//
//  Created by Jean-Pierre Mouilleseaux on 29 Jun 2015.
//  Copyright 2015 Chorded Constructions. All rights reserved.
//

#include "SerialDevice.h"

namespace Cinder { namespace Serial {

std::vector<SerialPortRef> SerialPort::sPorts;
bool SerialPort::sPortsDirty = true;

#pragma mark PORT
const std::vector<SerialPortRef>& SerialPort::getPorts(bool forceRefresh) {
    if (!forceRefresh && !sPortsDirty) {
        return sPorts;
    }

    sPorts.clear();

    for (auto info : serial::list_ports()) {
        auto port = SerialPort::create(info);
        sPorts.push_back(port);
    }

    sPortsDirty = false;

    return sPorts;
}

SerialPortRef SerialPort::findPortByNameMatching(const std::regex& pattern, bool forceRefresh) {
    if (forceRefresh || sPortsDirty) {
        getPorts(forceRefresh);
    }

    for (auto port : sPorts) {
        if (std::regex_match(port->getName(), pattern)) {
            return port;
        }
    }

    return nullptr;
}

const std::string SerialPort::getName() const {
    return mInfo.port;
}

const std::string SerialPort::getDescription() const {
    return mInfo.description != "n/a" ? mInfo.description : "";
}

const std::string SerialPort::getHardwareIdentifier() const {
    return mInfo.hardware_id != "n/a" ? mInfo.hardware_id : "";
}

#pragma mark DEVICE
SerialDeviceRef SerialDevice::create(const SerialPortRef port, uint32_t baudRate, const Timeout& timeout, DataBits dataBits, Parity parity, StopBits stopBits, FlowControl flowControl) {
    return SerialDeviceRef(new SerialDevice(port->getName(), baudRate, timeout, dataBits, parity, stopBits, flowControl))->shared_from_this();
}

SerialDeviceRef SerialDevice::create(const std::string& portName, uint32_t baudRate, const Timeout& timeout, DataBits dataBits, Parity parity, StopBits stopBits, FlowControl flowControl) {
    return SerialDeviceRef(new SerialDevice(portName, baudRate, timeout, dataBits, parity, stopBits, flowControl))->shared_from_this();
}

SerialDevice::SerialDevice(const std::string& portName, uint32_t baudRate, const Timeout& timeout, DataBits dataBits, Parity parity, StopBits stopBits, FlowControl flowControl) {
    mSerial = SerialRef(new serial::Serial(portName, baudRate, timeout, static_cast<serial::bytesize_t>(dataBits), static_cast<serial::parity_t>(parity), static_cast<serial::stopbits_t>(stopBits), static_cast<serial::flowcontrol_t>(flowControl)));
}

SerialDevice::~SerialDevice() {
    flushOutput();
    close();
}

const std::string SerialDevice::getPortName() const {
    return mSerial->getPort();
}

uint32_t SerialDevice::getBaudRate() const {
    return mSerial->getBaudrate();
}

const Timeout SerialDevice::getTimeout() const {
    return mSerial->getTimeout();
}

DataBits SerialDevice::getDataBits() const {
    return static_cast<DataBits>(mSerial->getBytesize());
}

Parity SerialDevice::getParity() const {
    return static_cast<Parity>(mSerial->getParity());
}

StopBits SerialDevice::getStopBits() const {
    return static_cast<StopBits>(mSerial->getStopbits());
}

FlowControl SerialDevice::getFlowControl() const {
    return static_cast<FlowControl>(mSerial->getFlowcontrol());
}

bool SerialDevice::isOpen() const {
    return mSerial->isOpen();
}

void SerialDevice::open() {
    if (isOpen()) {
        return;
    }

    mSerial->open();
}

void SerialDevice::close() {
    if (!isOpen()) {
        return;
    }

    mSerial->close();
}

size_t SerialDevice::getNumBytesAvailable() const {
    return mSerial->available();
}

size_t SerialDevice::readBytes(uint8_t* buffer, size_t maxSize) {
    return mSerial->read(buffer, maxSize);
}

size_t SerialDevice::writeBytes(const uint8_t* buffer, size_t size) {
    return mSerial->write(buffer, size);
}

void SerialDevice::flush() {
    mSerial->flush();
}

void SerialDevice::flushInput() {
    mSerial->flushInput();
}

void SerialDevice::flushOutput() {
    mSerial->flushOutput();
}

}}
