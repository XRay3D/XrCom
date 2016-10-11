#include "available_ports.h"

#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "advapi32.lib")

AVAILABLE_PORTS::AVAILABLE_PORTS()
{
    string dev;
    GUID GUID_CLASS_COMPORT = { 0x4D36E978, 0xE325, 0x11CE, 0xBF, 0xC1, 0x08, 0x00, 0x2B, 0xE1, 0x03, 0x18 };
    const HDEVINFO deviceInfoSet = ::SetupDiGetClassDevsA(&GUID_CLASS_COMPORT, nullptr, nullptr, DIGCF_PRESENT);
    if (deviceInfoSet == INVALID_HANDLE_VALUE)
        return;
    SP_DEVINFO_DATA deviceInfoData;
    ::memset(&deviceInfoData, 0, sizeof(deviceInfoData));
    deviceInfoData.cbSize = sizeof(deviceInfoData);

    DWORD index = 0;
    while (::SetupDiEnumDeviceInfo(deviceInfoSet, index++, &deviceInfoData)) {
        string portname = devicePortName(deviceInfoSet, &deviceInfoData);
        if (portname.empty() || portname.find("LPT") == 0)
            continue;
        dev = "\\\\.\\";
        portName.push_back(portname);
        device.push_back((dev += portname));
        description.push_back(deviceDescription(deviceInfoSet, &deviceInfoData));
        manufacturer.push_back(deviceManufacturer(deviceInfoSet, &deviceInfoData));
        string instanceidentifier = deviceInstanceIdentifier(deviceInfoData.DevInst);
        bool has;
        instanceIdentifier.push_back(instanceidentifier);
        serialNumber.push_back(deviceSerialNumber(instanceidentifier, deviceInfoData.DevInst));
        vendorIdentifier.push_back(deviceVendorIdentifier(instanceidentifier, has));
        productIdentifier.push_back(deviceProductIdentifier(instanceidentifier, has));
    }
    ::SetupDiDestroyDeviceInfoList(deviceInfoSet);
}

string AVAILABLE_PORTS::deviceRegistryProperty(HDEVINFO deviceInfoSet, PSP_DEVINFO_DATA deviceInfoData, DWORD property)
{
    DWORD dataType = 0;
    string devicePropertyByteArray;
    DWORD requiredSize = 0;
    while (1) {
        if (::SetupDiGetDeviceRegistryPropertyA(deviceInfoSet, deviceInfoData, property, &dataType, (unsigned char*)devicePropertyByteArray.data(), devicePropertyByteArray.size(), &requiredSize)) {
            break;
        }

        if (::GetLastError() != ERROR_INSUFFICIENT_BUFFER || (dataType != REG_SZ && dataType != REG_EXPAND_SZ)) {
            return string();
        }
        devicePropertyByteArray.resize(requiredSize);
    }
    return devicePropertyByteArray;
}

string AVAILABLE_PORTS::deviceManufacturer(HDEVINFO deviceInfoSet, PSP_DEVINFO_DATA deviceInfoData)
{
    return deviceRegistryProperty(deviceInfoSet, deviceInfoData, SPDRP_MFG);
}

string AVAILABLE_PORTS::deviceDescription(HDEVINFO deviceInfoSet, PSP_DEVINFO_DATA deviceInfoData)
{
    return deviceRegistryProperty(deviceInfoSet, deviceInfoData, SPDRP_DEVICEDESC);
}

string AVAILABLE_PORTS::deviceInstanceIdentifier(DEVINST deviceInstanceNumber)
{
    ULONG numberOfChars = 0;
    if (::CM_Get_Device_ID_Size(&numberOfChars, deviceInstanceNumber, 0) != CR_SUCCESS)
        return string();
    // The size does not include the terminating null character.
    ++numberOfChars;
    string outputBuffer;
    outputBuffer.resize(numberOfChars * sizeof(char));
    if (::CM_Get_Device_IDA(deviceInstanceNumber, (char*)outputBuffer.data(), outputBuffer.size(), 0)
        != CR_SUCCESS) {
        return string();
    }
    return outputBuffer;
}

DEVINST AVAILABLE_PORTS::parentDeviceInstanceNumber(DEVINST childDeviceInstanceNumber)
{
    ULONG nodeStatus = 0;
    ULONG problemNumber = 0;
    if (::CM_Get_DevNode_Status(&nodeStatus, &problemNumber,
            childDeviceInstanceNumber, 0)
        != CR_SUCCESS) {
        return 0;
    }
    DEVINST parentInstanceNumber = 0;
    if (::CM_Get_Parent(&parentInstanceNumber, childDeviceInstanceNumber, 0) != CR_SUCCESS)
        return 0;
    return parentInstanceNumber;
}

string AVAILABLE_PORTS::parseDeviceSerialNumber(const string& instanceIdentifier)
{
    string instanceIdentifier2 = instanceIdentifier.substr(0, strlen(instanceIdentifier.data()));
    int firstbound = instanceIdentifier2.rfind /*lastIndexOf*/ ('\\');
    int lastbound = instanceIdentifier2.rfind /*indexOf*/ ('_', firstbound);
    if (instanceIdentifier2.find /*startsWith*/ ("USB\\") == 0) {
        if (lastbound != instanceIdentifier2.size() - 3)
            lastbound = instanceIdentifier2.size();
        int ampersand = instanceIdentifier2.find /*indexOf*/ ('&', firstbound);
        if (ampersand != -1 && ampersand < lastbound)
            return string();
    }
    else if (instanceIdentifier2.find /*startsWith*/ ("FTDIBUS\\") == 0) {
        firstbound = instanceIdentifier2.rfind /*lastIndexOf*/ ('+');
        lastbound = instanceIdentifier2.find /*indexOf*/ ('\\', firstbound);
        if (lastbound == -1)
            return string();
    }
    else {
        return string();
    }

    return instanceIdentifier2.substr /*mid*/ (firstbound + 1, lastbound - firstbound - 1);
}

string AVAILABLE_PORTS::deviceSerialNumber(const string& instanceIdentifier, DEVINST deviceInstanceNumber)
{
    string result = parseDeviceSerialNumber(instanceIdentifier);
    if (result.empty()) {
        const DEVINST parentNumber = parentDeviceInstanceNumber(deviceInstanceNumber);
        const string parentInstanceIdentifier = deviceInstanceIdentifier(parentNumber);
        result = parseDeviceSerialNumber(parentInstanceIdentifier);
    }
    return result;
}

/*quint16*/ string AVAILABLE_PORTS::parseDeviceIdentifier(const string& instanceIdentifier, const string& identifierPrefix, int identifierSize, bool& ok)
{
    const int index = instanceIdentifier.find /*indexOf*/ (identifierPrefix);
    if (index == -1)
        return string(); //quint16(0);
    string hex = instanceIdentifier.substr(index + identifierPrefix.size(), identifierSize);
    try {
        unsigned short x = stoul(hex, nullptr, 16);
        ok = true;
        return hex;
    }
    catch (invalid_argument&) {
        ok = false;
        return string();
    }
}

/*quint16*/ string AVAILABLE_PORTS::deviceVendorIdentifier(const string& instanceIdentifier, bool& ok)
{
    static const int vendorIdentifierSize = 4;
    string /*quint16*/ result = parseDeviceIdentifier(instanceIdentifier, "VID_", vendorIdentifierSize, ok);
    if (!ok)
        result = parseDeviceIdentifier(instanceIdentifier, "VEN_", vendorIdentifierSize, ok);
    return result;
}

/*quint16*/ string AVAILABLE_PORTS::deviceProductIdentifier(const string& instanceIdentifier, bool& ok)
{
    static const int productIdentifierSize = 4;
    string /*quint16*/ result = parseDeviceIdentifier(instanceIdentifier, "PID_", productIdentifierSize, ok);
    if (!ok)
        result = parseDeviceIdentifier(instanceIdentifier, "DEV_", productIdentifierSize, ok);
    return result;
}

string AVAILABLE_PORTS::devicePortName(HDEVINFO deviceInfoSet, PSP_DEVINFO_DATA deviceInfoData)
{
    const HKEY key = ::SetupDiOpenDevRegKey(deviceInfoSet, deviceInfoData, DICS_FLAG_GLOBAL,
        0, DIREG_DEV, KEY_READ);
    if (key == INVALID_HANDLE_VALUE)
        return string();

    const char* portNameRegistryKeyList[] = { "PortName", "PortNumber" };

    string portName;
    for (int i = 0; i < 2; ++i) {
        DWORD bytesRequired = 0;
        DWORD dataType = 0;
        string outputBuffer;
        while (1) {
            const LONG ret = ::RegQueryValueExA(key, portNameRegistryKeyList[i], nullptr, &dataType, (unsigned char*)outputBuffer.data(), &bytesRequired);
            if (ret == ERROR_MORE_DATA) {
                outputBuffer.resize(bytesRequired);
                continue;
            }
            else if (ret == ERROR_SUCCESS) {
                if (dataType == REG_SZ) {
                    portName = outputBuffer.data();
                }
                else if (dataType == REG_DWORD) {
                    char data[7];
                    sprintf(data, "COM%u", *(PDWORD(outputBuffer.data())));
                    portName = data;
                }
            }
            break;
        }
        if (!portName.empty())
            break;
    }
    ::RegCloseKey(key);
    return portName;
}
