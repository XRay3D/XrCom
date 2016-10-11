#include "comport.h"

COMPORT::COMPORT()
    : MyEvFunc(0)
    , MyRdFunc(0)
    , hRdThr(0)
    , hWrThr(0)
    , hComport(0)
    , Data(0)
    , dwWrFlag(0)
{
    RdThread.Method = &COMPORT::ReadThread; //it's magic
    WrThread.Method = &COMPORT::WriteThread; //it's magic
}

COMPORT::~COMPORT()
{
    CpClose();
}

long COMPORT::CpOpen(HANDLE* Handle, DWORD Speed, DWORD EvtFlag, long EvtChar, LPCSTR PortName)
{
    dwEvRdFlag = EvtFlag;
    DCB dcb;
    COMMTIMEOUTS timeouts;
    if (!hComport) {
        hComport = ::CreateFileA(PortName, GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr);
        //если ошибка открытия порта
        if (hComport == INVALID_HANDLE_VALUE) {
            return -1;
        }
        dcb.DCBlength = sizeof(DCB);
        if (!::GetCommState(hComport, &dcb)) {
            CpClose();
            return -2;
        }
        dcb.BaudRate = Speed; //задаём скорость передачи X бод
        dcb.fBinary = TRUE; //включаем двоичный режим обмена
        dcb.fOutxCtsFlow = FALSE; //выключаем режим слежения за сигналом CTS
        dcb.fOutxDsrFlow = FALSE; //выключаем режим слежения за сигналом DSR
        dcb.fDtrControl = DTR_CONTROL_DISABLE; //отключаем использование линии DTR
        dcb.fDsrSensitivity = FALSE; //отключаем восприимчивость драйвера к состоянию линии DSR
        dcb.fNull = FALSE; //разрешить приём нулевых байтов
        dcb.fRtsControl = RTS_CONTROL_DISABLE; //отключаем использование линии RTS
        dcb.fAbortOnError = FALSE; //отключаем остановку всех операций чтения/записи при ошибке
        dcb.ByteSize = 8; //задаём 8 бит в байте
        dcb.Parity = 0; //отключаем проверку чётности
        dcb.StopBits = 0;
        dcb.EvtChar = (char)EvtChar;
        //загрузить структуру DCB в порт
        //если не удалось - закрыть порт и вывести сообщение об ошибке в строке состояния
        if (!::SetCommState(hComport, &dcb)) {
            CpClose();
            return -3;
        }
        //установить таймауты
        timeouts.ReadIntervalTimeout = 100; //таймаут между двумя символами
        timeouts.ReadTotalTimeoutMultiplier = 1; //общий таймаут операции чтения
        timeouts.ReadTotalTimeoutConstant = 1; //константа для общего таймаута операции чтения
        timeouts.WriteTotalTimeoutMultiplier = 0; //общий таймаут операции записи
        timeouts.WriteTotalTimeoutConstant = 0; //константа для общего таймаута операции записи
        //записать структуру таймаутов в порт
        //если не удалось - закрыть порт и вывести сообщение об ошибке в строке состояния
        if (!::SetCommTimeouts(hComport, &timeouts)) {
            CpClose();
            return -4;
        }
        ::SetupComm(hComport, 0xFFF, 0xFFF); //установить размеры очередей приёма и передачи
        ::PurgeComm(hComport, PURGE_RXCLEAR); //очистить принимающий буфер порта
        Data = ::SafeArrayCreateVector(VT_UI1, 0, BufRd.size());
        hRdThr = ::CreateThread(NULL, 500, RdThread.Function, this, 0, NULL); //создаём поток чтения, который сразу начнёт выполняться (предпоследний параметр = 0)
        hWrThr = ::CreateThread(NULL, 500, WrThread.Function, this, CREATE_SUSPENDED, NULL); //создаём поток записи в остановленном состоянии (предпоследний параметр = CREATE_SUSPENDED)
        if (!Data || !hRdThr || !hWrThr) {
            CpClose();
            return -5;
        }
        *Handle = hComport;
        /*hMutexWr = CreateMutex(
            NULL,              // default security attributes
            FALSE,             // initially not owned
            NULL);             // unnamed mutex
        hMutexRd = CreateMutex(
            NULL,              // default security attributes
            FALSE,             // initially not owned
            NULL);             // unnamed mutex*/
    }
    return 1;
}

long COMPORT::CpClose()
{
    long FLAG;
    if (hComport > 0) {
        if (hWrThr > 0) //если поток записи работает, завершить его; проверка if обязательна, иначе возникают ошибки
        {
            ::TerminateThread(hWrThr, NULL);
            ::CloseHandle(OlWr.hEvent); //нужно закрыть объект-событие
            //WaitForSingleObject(WR_THR._HANDLE, INFINITE);
            ::CloseHandle(hWrThr);
            hWrThr = 0;
        }
        if (hRdThr > 0) //если поток чтения работает, завершить его; проверка if обязательна, иначе возникают ошибки
        {
            ::TerminateThread(hRdThr, NULL);
            ::CloseHandle(OlRd.hEvent); //нужно закрыть объект-событие
            //WaitForSingleObject(RD_THR._HANDLE, INFINITE);
            ::CloseHandle(hRdThr);
            hRdThr = 0;
        }
        FLAG = ::CloseHandle(hComport); //закрыть порт
        hComport = 0; //обнулить переменную дескриптора порта
        SafeArrayDestroy(Data);
        Data = 0;
    }
    return FLAG;
}

long COMPORT::CpWrite(SAFEARRAY** Data)
{
    long iLBound = 0, iUBound = 0;
    HRESULT hr;
    if (!*Data) //Проверка данных
        return E_FAIL;
    hr = ::SafeArrayGetLBound(*Data, 1, &iLBound); //Получение нижней граници
    if (FAILED(hr))
        return hr;
    hr = ::SafeArrayGetUBound(*Data, 1, &iUBound); //Получение верхней граници
    if (FAILED(hr))
        return hr;
    BYTE* pData = NULL;
    hr = ::SafeArrayAccessData(*Data, (void**)&pData); //Получение указателя на данные
    if (FAILED(hr))
        return hr;

    //    while (bWrFlag--) //Ожидание прредыдущей записи
    //        ::Sleep(1);
    //    bWrFlag = 10000; //10 секунд

    BufWr.resize(iUBound - iLBound + 1); //Изменение размера буфера передачи
    memcpy(BufWr.data(), pData, BufWr.size()); //копированние данных в буфер
    ::SafeArrayUnaccessData(*Data); //"Освобождение"
    ::ResumeThread(hWrThr); //Запуск записи
    return BufWr.size(); //Возврат количества записанных байт
}

long COMPORT::CpRead(SAFEARRAY** Data)
{
    long len;
    HRESULT hr;
    BYTE* pData;
    pData = NULL;

    //Есть что читать
    if (BufRd.size()) {
        //Есть "куда возвращать"
        if (*Data) {
            SAFEARRAYBOUND saBound;
            saBound.lLbound = 0;
            saBound.cElements = BufRd.size();
            //Переразмер  "куда возвращать"
            hr = SafeArrayRedim(*Data, &saBound);
            if (FAILED(hr))
                return hr;
        }
        else {
            //Создаём  "куда возвращать"
            *Data = ::SafeArrayCreateVector(VT_UI1, 0, BufRd.size());
            if (!*Data) //Проверка  "куда возвращать"
                return -2;
        }
        //Получение указателя на данные
        hr = ::SafeArrayAccessData(*Data, (void**)&pData);
        if (FAILED(hr))
            return hr;
        //копированние данных в буфер
        ::memcpy(pData, BufRd.data(), BufRd.size());
        //"Освобождение"
        hr = ::SafeArrayUnaccessData(*Data);
        if (FAILED(hr))
            return hr;
        len = BufRd.size();
        BufRd.clear();
        return len;
    }
    else
        return -1;
}

long COMPORT::CpRTS(bool FLAG)
{
    if (FLAG)
        return ::EscapeCommFunction(hComport, SETRTS);
    else
        return ::EscapeCommFunction(hComport, CLRRTS);
}

long COMPORT::CpDTR(bool FLAG)
{
    if (FLAG)
        return ::EscapeCommFunction(hComport, SETDTR);
    else
        return ::EscapeCommFunction(hComport, CLRDTR);
}

long COMPORT::CpReadFunc(pRdFuncType AddrFunc)
{
    MyRdFunc = AddrFunc;
    return long(MyRdFunc);
}

long COMPORT::CpEventFunc(pEvFuncType AddrFunc)
{
    MyEvFunc = AddrFunc;
    return long(MyEvFunc);
}

void COMPORT::setReadyRead(bool *value)
{
    readyRead = value;
}

DWORD COMPORT::WriteThread(LPVOID)
{
    //temp - переменная-заглушка
    DWORD temp, signal;
    //создать событие
    OlWr.hEvent = ::CreateEventA(NULL, true, true, NULL);
    while (1) {
        //записать байты в порт (перекрываемая операция!)
        ::WriteFile(hComport, BufWr.data(), BufWr.size(), &temp, &OlWr);
        //приостановить поток, пока не завершится перекрываемая операция WriteFile
        signal = ::WaitForSingleObject(OlWr.hEvent, INFINITE);
        //если операция завершилась успешно
        if ((signal == WAIT_OBJECT_0) && (::GetOverlappedResult(hComport, &OlWr, &temp, true)))
            dwWrFlag = 1;
        else
            dwWrFlag = -1;
        ::SuspendThread(hWrThr);
    }
    return 0;
}

DWORD COMPORT::ReadThread(LPVOID)
{
    BYTE* pData = NULL;
    //структура текущего состояния порта
    //в данной программе используется для определения количества принятых в порт байтов
    COMSTAT comstat;
    //переменная temp используется в качестве заглушки
    DWORD dwMask, dwSignal, PinStatus, temp, dwByteRead;
    //создать сигнальный объект-событие для асинхронных операций
    OlRd.hEvent = ::CreateEventA(NULL, true, true, NULL);
    //установить маску на срабатывание по событию приёма байта в порт
    ::SetCommMask(hComport, dwEvRdFlag);
    //пока поток не будет прерван, выполняем цикл
    while (1) {
        dwByteRead = 0;
        //ожидать события приёма байта (это и есть перекрываемая операция)
        ::WaitCommEvent(hComport, &dwMask, &OlRd);
        //приостановить поток до прихода байта
        dwSignal = ::WaitForSingleObject(OlRd.hEvent, INFINITE);
        //если событие прихода байта произошло
        if (dwSignal == WAIT_OBJECT_0) {
            //проверяем, успешно ли завершилась перекрываемая операция WaitCommEvent
            if (::GetOverlappedResult(hComport, &OlRd, &temp, true)) {
                //если произошло именно событие прихода байта
                if ((dwMask & dwEvRdFlag) != 0) {
                    //Отслеживание других линий
                    if (MyEvFunc != 0) {
                        ::GetCommModemStatus(hComport, &PinStatus);
                        (*MyEvFunc)(dwMask, PinStatus); //Вызвать каллбак события
                    }
                    //нужно заполнить структуру COMSTAT
                    ::ClearCommError(hComport, &temp, &comstat);
                    //и получить из неё количество принятых байтов
                    dwByteRead = comstat.cbInQue;
                    //если действительно есть байты для чтения
                    if (dwByteRead) {
                        BufRd.resize(BufRd.size() + dwByteRead, 0);
                        char* data = BufRd.data() + (BufRd.size() - dwByteRead);
                        //прочитать байты из порта в буфер программы
                        ::ReadFile(hComport, data, dwByteRead, &temp, &OlRd);
                        //Если есть фунция "калбак"
                        if (readyRead)
                            *readyRead = false;
                        if (MyRdFunc != 0) {
                            if (Data) {
                                SAFEARRAYBOUND saBound;
                                saBound.lLbound = 0;
                                saBound.cElements = BufRd.size();
                                //Переразмер  "куда возвращать"
                                if (SUCCEEDED(::SafeArrayRedim(Data, &saBound))) {
                                    if (SUCCEEDED(::SafeArrayAccessData(Data, (void**)&pData))) {
                                        ::memcpy(pData, BufRd.data(), BufRd.size());
                                        ::SafeArrayUnaccessData(Data);
                                        BufRd.clear();
                                        (*MyRdFunc)(&Data);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return 0;
}
//////////////////////////////////////////////
