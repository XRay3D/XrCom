#ifndef AVAILABLE_PORTS_H
#define AVAILABLE_PORTS_H

#include <iostream>

#include <vector>
#include <string>

#include <Windows.h>
#include <SetupAPI.h>
#include <cfgmgr32.h>

using namespace std;

class AVAILABLE_PORTS {
public:
    AVAILABLE_PORTS();

    vector<string>& ProductIdentifier() { return productIdentifier; }
    vector<string>& VendorIdentifier() { return vendorIdentifier; }
    vector<string>& SerialNumber() { return serialNumber; }
    vector<string>& InstanceIdentifier() { return instanceIdentifier; }
    vector<string>& Manufacturer() { return manufacturer; }
    vector<string>& Description() { return description; }
    vector<string>& Device() { return device; }
    vector<string>& PortName() { return portName; }

private:
    static string deviceRegistryProperty(HDEVINFO deviceInfoSet, PSP_DEVINFO_DATA deviceInfoData, DWORD property);
    static string deviceManufacturer(HDEVINFO deviceInfoSet, PSP_DEVINFO_DATA deviceInfoData);
    static string deviceDescription(HDEVINFO deviceInfoSet, PSP_DEVINFO_DATA deviceInfoData);
    static string deviceInstanceIdentifier(DEVINST deviceInstanceNumber);
    static DEVINST parentDeviceInstanceNumber(DEVINST childDeviceInstanceNumber);
    static string parseDeviceSerialNumber(const string& instanceIdentifier);
    static string deviceSerialNumber(const string& instanceIdentifier, DEVINST deviceInstanceNumber);
    static /*quint16*/ string parseDeviceIdentifier(const string& instanceIdentifier, const string& identifierPrefix, int identifierSize, bool& ok);
    static /*quint16*/ string deviceVendorIdentifier(const string& instanceIdentifier, bool& ok);
    static /*quint16*/ string deviceProductIdentifier(const string& instanceIdentifier, bool& ok);
    static string devicePortName(HDEVINFO deviceInfoSet, PSP_DEVINFO_DATA deviceInfoData);

    vector<string> portName;
    vector<string> device;
    vector<string> description;
    vector<string> manufacturer;
    vector<string> instanceIdentifier;
    vector<string> serialNumber;
    vector<string> vendorIdentifier;
    vector<string> productIdentifier;
};

#endif // AVAILABLE_PORTS_H
