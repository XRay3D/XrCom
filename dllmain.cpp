#include "comport.h"
#include "available_ports.h"

#include <vector>
#include <map>

#include <Windows.h>
#include <SetupAPI.h>
#pragma comment(lib, "setupapi.lib")
#define EXPORT __declspec(dllexport)

using namespace std;

map<HANDLE, COMPORT*>::iterator it;
map<HANDLE, COMPORT*> ComPortMap; //Map из компортов с двумя потоками в каждом, да и ещё с двумя калбаками.
vector<string> Addres;
EXPORT long __stdcall McpTest(SAFEARRAY** String, bool FullInfo = false)
{
    String;
    FullInfo;
    return 0;
}

EXPORT long __stdcall McpEnum2(SAFEARRAY** String, bool FullInfo = false)
{
    HRESULT hresult;
    //    long LBound = 0, UBound = 0;
    AVAILABLE_PORTS ports;
    LPSAFEARRAY pSa;

    if (ports.PortName().size() < 1)
        return 0;

    Addres = ports.Device();

    if (FullInfo) {
        SAFEARRAYBOUND rgSaBound[2];
        rgSaBound[0].lLbound = 0;
        rgSaBound[0].cElements = ports.PortName().size();
        rgSaBound[1].lLbound = 0;
        rgSaBound[1].cElements = 8;

        pSa = SafeArrayCreate(VT_BSTR, 2, rgSaBound);
        if (pSa == NULL)
            return -1;

        BSTR* pString = NULL;
        hresult = SafeArrayAccessData(pSa, (void**)&pString);
        if (FAILED(hresult))
            return -2;

        for (size_t i = 0; i < ports.PortName().size(); ++i) {
            pString[i + ports.PortName().size() * 0] = SysAllocStringByteLen(ports.PortName().at(i).data(), ports.PortName().at(i).length());
            pString[i + ports.PortName().size() * 1] = SysAllocStringByteLen(ports.Device().at(i).data(), ports.Device().at(i).length());
            pString[i + ports.PortName().size() * 2] = SysAllocStringByteLen(ports.Description().at(i).data(), ports.Description().at(i).length());
            pString[i + ports.PortName().size() * 3] = SysAllocStringByteLen(ports.Manufacturer().at(i).data(), ports.Manufacturer().at(i).length());
            pString[i + ports.PortName().size() * 4] = SysAllocStringByteLen(ports.InstanceIdentifier().at(i).data(), ports.InstanceIdentifier().at(i).length());
            pString[i + ports.PortName().size() * 5] = SysAllocStringByteLen(ports.SerialNumber().at(i).data(), ports.SerialNumber().at(i).length());
            pString[i + ports.PortName().size() * 6] = SysAllocStringByteLen(ports.VendorIdentifier().at(i).data(), ports.VendorIdentifier().at(i).length());
            pString[i + ports.PortName().size() * 7] = SysAllocStringByteLen(ports.ProductIdentifier().at(i).data(), ports.ProductIdentifier().at(i).length());
        }
        hresult = SafeArrayUnaccessData(pSa);
        if (FAILED(hresult))
            return -3;
    }
    else {
        pSa = SafeArrayCreateVector(VT_BSTR, 0, ports.PortName().size());
        if (pSa == NULL)
            return -1;

        BSTR* pString = NULL;
        hresult = SafeArrayAccessData(pSa, (void**)&pString);
        if (FAILED(hresult))
            return -2;

        for (size_t i = 0; i < ports.PortName().size(); ++i) {
            pString[i] = SysAllocStringByteLen(LPCSTR(ports.PortName().at(i).data()), ports.PortName().at(i).length());
        }

        hresult = SafeArrayUnaccessData(pSa);
        if (FAILED(hresult))
            return -3;
    }
    SafeArrayDestroy(*String);
    *String = pSa;

    return ports.PortName().size();
}

EXPORT long __stdcall McpEnum(SAFEARRAY** String)
{
    return McpEnum2(String);
}

EXPORT HANDLE __stdcall McpOpen(int Index, DWORD Speed = 9600, DWORD EvtFlag = 1, long EvtChar = 0)
{
    COMPORT* CP = new COMPORT();
    HANDLE h = 0;
    long Result;
    if (Addres.size() < 1)
        return h;
    Result = CP->CpOpen(&h, Speed, EvtFlag, EvtChar, Addres.at(Index).data());
    if (Result > 0) {
        ComPortMap[h] = CP;
        return h;
    }
    else
        return HANDLE(Result);
}

EXPORT long __stdcall McpClose(HANDLE h)
{
    long Result;
    it = ComPortMap.find(h);
    if (it != ComPortMap.end()) {
        Result = it->second->CpClose();
        delete it->second;
        ComPortMap.erase(it);
        return Result;
    }
    else
        return -10;
}

EXPORT long __stdcall McpSetRTS(HANDLE h)
{
    it = ComPortMap.find(h);
    if (it != ComPortMap.end())
        return it->second->CpRTS(true);
    else
        return -10;
}

EXPORT long __stdcall McpSetDTR(HANDLE h)
{
    it = ComPortMap.find(h);
    if (it != ComPortMap.end())
        return it->second->CpDTR(true);
    else
        return -10;
}

EXPORT long __stdcall McpClrRTS(HANDLE h)
{
    it = ComPortMap.find(h);
    if (it != ComPortMap.end())
        return it->second->CpRTS(false);
    else
        return -10;
}

EXPORT long __stdcall McpClrDTR(HANDLE h)
{
    it = ComPortMap.find(h);
    if (it != ComPortMap.end())
        return it->second->CpDTR(false);
    else
        return -10;
}

EXPORT long __stdcall McpWrite(HANDLE h, SAFEARRAY** Data)
{
    it = ComPortMap.find(h);
    if (it != ComPortMap.end())
        return it->second->CpWrite(Data);
    else
        return -10;
}

EXPORT long __stdcall McpWrite2(HANDLE h, SAFEARRAY** Data, bool* readyRead = 0)
{
    it = ComPortMap.find(h);
    if (it != ComPortMap.end()) {
        it->second->setReadyRead(readyRead);
        return it->second->CpWrite(Data);
    }
    else
        return -10;
}

EXPORT long __stdcall McpRead(HANDLE h, SAFEARRAY** Data)
{
    it = ComPortMap.find(h);
    if (it != ComPortMap.end()) {
        it->second->setReadyRead(0);
        return it->second->CpRead(Data);
    }
    else
        return -10;
}

EXPORT long __stdcall McpEventFunc(HANDLE h, pEvFuncType AddrFunc)
{
    it = ComPortMap.find(h);
    if (it != ComPortMap.end())
        return it->second->CpEventFunc(AddrFunc);
    else
        return -10;
}

EXPORT long __stdcall McpReadFunc(HANDLE h, pRdFuncType AddrFunc)
{
    it = ComPortMap.find(h);
    if (it != ComPortMap.end())
        return it->second->CpReadFunc(AddrFunc);
    else
        return -10;
}

BOOL APIENTRY DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    hinstDLL;
    lpvReserved;
    switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
        Addres.clear();
        ComPortMap.clear();
        break;
    case DLL_THREAD_ATTACH:
        break;
    case DLL_THREAD_DETACH:
        break;
    case DLL_PROCESS_DETACH:
        //Addres.clear();
        it = ComPortMap.begin();
        while (it != ComPortMap.end()) {
            it->second->CpClose();
            delete it->second;
            it++;
        }
        break;
    }
    return TRUE;
}
