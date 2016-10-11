#ifndef COM_PORT_H
#define COM_PORT_H

#include <windows.h>
#include <iostream>

#include <atlbase.h>
#include <atlcom.h>
#include <atlctl.h>

#include <vector>

using namespace std;

typedef void(__stdcall* pRdFuncType)(SAFEARRAY**);
typedef void(__stdcall* pEvFuncType)(const long, const long);

class COMPORT {
public:
    COMPORT();
    ~COMPORT();
    long CpOpen(HANDLE* Handle, DWORD Speed, DWORD EvtFlag, long EvtChar, LPCSTR PortName);
    long CpClose();
    long CpWrite(SAFEARRAY** Data);
    long CpRead(SAFEARRAY** Data);
    long CpRTS(bool FLAG);
    long CpDTR(bool FLAG);
    long CpReadFunc(pRdFuncType AddrFunc);
    long CpEventFunc(pEvFuncType AddrFunc);

    void setReadyRead(bool* value);

private:
    bool* readyRead;

    pRdFuncType MyRdFunc; //Callback'i
    pEvFuncType MyEvFunc;

    DWORD dwEvRdFlag;
    HANDLE hComport;

    DWORD dwWrFlag; //флаг, указывающий на успешность операций записи (1 - успешно, 0 - не успешно)

    SAFEARRAY* Data; //Буфер для калбака

    vector<char> BufWr; //Передающий буфер
    vector<char> BufRd; //Принимающий буфер

    //    HANDLE hMutexWr;
    //    HANDLE hMutexRd;

    // объявляем необходимые типы
    // прототип функции потока
    typedef DWORD(WINAPI* ThrdFunc)(LPVOID);
    // прототип метода класса
    typedef DWORD (WINAPI COMPORT::*ClassMethod)(LPVOID);
    // данное объединение позволяет решить несостыковку с типами
    typedef union {
        ThrdFunc Function;
        ClassMethod Method;
    } tThrdAddr;

    tThrdAddr RdThread;
    tThrdAddr WrThread;

    HANDLE hRdThr;
    HANDLE hWrThr;

    OVERLAPPED OlRd; //будем использовать для операций чтения (см. поток ReadThread)
    OVERLAPPED OlWr; //будем использовать для операций записи (см. поток WriteThread)

    virtual DWORD WINAPI WriteThread(LPVOID);
    virtual DWORD WINAPI ReadThread(LPVOID);
};

#endif // COM_PORT_H
