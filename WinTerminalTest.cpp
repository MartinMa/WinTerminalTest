#include <iostream>
#include <windows.h>
#include <tchar.h>
#include <Shlobj_core.h>
#include <pathcch.h>

// Global constants.
const SHORT CONSOLE_HEIGHT = 25;
const SHORT CONSOLE_WIDTH = 80;

// Global variables.
HANDLE g_hFile;
HANDLE g_hInputConsole = NULL;
HANDLE g_hOutputConsole = NULL;
HWINEVENTHOOK g_hook;

void printLastError(const TCHAR* format)
{
    TCHAR message[1024];
    _stprintf_s(message, 1024, format, GetLastError());
    OutputDebugString(message);
}

void outputLogMessage(const TCHAR* message) {
#if _DEBUG
    OutputDebugString(message);
#else
    DWORD dwBytesWritten = 0;
    WriteFile(
        g_hFile,
        message,
        lstrlenW(message) * sizeof(TCHAR),
        &dwBytesWritten,
        NULL
    );
#endif
}

void CALLBACK HandleWinEvent(HWINEVENTHOOK hook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime)
{
    switch (event) {
    case EVENT_CONSOLE_CARET:
    {
        TCHAR output[1024];
        std::basic_string<TCHAR> flags;

        COORD coord;
        coord.X = LOWORD(idChild);
        coord.Y = HIWORD(idChild);

        if (idObject & CONSOLE_CARET_SELECTION) {
            flags += _T("CONSOLE_CARET_SELECTION");
        }
        if (idObject & CONSOLE_CARET_VISIBLE) {
            if (flags.length() > 0) {
                flags += _T(", CONSOLE_CARET_VISIBLE");
            }
            else {
                flags += _T("CONSOLE_CARET_VISIBLE");
            }
        }

        _stprintf_s(output, 1024, _T("EVENT_CONSOLE_CARET: [X: %d, Y: %d, Flags: %s]\n"), coord.X, coord.Y, flags.c_str());
        outputLogMessage(output);
        break;
    }
    case EVENT_CONSOLE_UPDATE_REGION:
    {
        TCHAR output[1024];

        COORD coordStart;
        coordStart.X = LOWORD(idObject);
        coordStart.Y = HIWORD(idObject);

        COORD coordEnd;
        coordEnd.X = LOWORD(idChild);
        coordEnd.Y = HIWORD(idChild);

        _stprintf_s(output, 1024, _T("EVENT_CONSOLE_UPDATE_REGION: [Left: %d, Top: %d, Right: %d, Bottom: %d]\n"), coordStart.X, coordStart.Y, coordStart.X, coordEnd.Y);
        outputLogMessage(output);
        break;
    }
    case EVENT_CONSOLE_UPDATE_SIMPLE:
    {
        TCHAR output[1024];

        COORD coord;
        coord.X = LOWORD(idObject);
        coord.Y = HIWORD(idObject);

        SHORT chUpdate = LOWORD(idChild);
        SHORT wAttributes = HIWORD(idChild);

        _stprintf_s(output, 1024, _T("EVENT_CONSOLE_UPDATE_SIMPLE: [X: %d, X: %d, Char: %d, Attr: %d]\n"), coord.X, coord.Y, chUpdate, wAttributes);
        outputLogMessage(output);
        break;
    }
    case EVENT_CONSOLE_UPDATE_SCROLL:
    {
        TCHAR output[1024];
        _stprintf_s(output, 1024, _T("EVENT_CONSOLE_UPDATE_SCROLL: [dx: %d, dy: %d]\n"), idObject, idChild);
        outputLogMessage(output);
        break;
    }
    case EVENT_CONSOLE_LAYOUT:
        outputLogMessage(_T("EVENT_CONSOLE_LAYOUT\n"));
        break;
    case EVENT_CONSOLE_START_APPLICATION:
    {
        TCHAR output[1024];
        _stprintf_s(output, 1024, _T("EVENT_CONSOLE_START_APPLICATION: [Process ID: %u]\n"), idObject);
        outputLogMessage(output);
        break;
    }
    case EVENT_CONSOLE_END_APPLICATION:
    {
        TCHAR output[1024];
        _stprintf_s(output, 1024, _T("EVENT_CONSOLE_END_APPLICATION: [Process ID: %u]\n"), idObject);
        outputLogMessage(output);
        break;
    }
    default:
        outputLogMessage(_T("UNKNOWN EVENT\n"));
        break;
    }
}

int main()
{
    std::cout << "Console Event Test!\n";

#if !_DEBUG
    g_hFile = CreateFile(
        _T("logfile.txt"),
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (g_hFile == INVALID_HANDLE_VALUE) {
        printLastError(_T("CreateFile failed with %d\n"));
        return 1;
    }
#endif

    if (!FreeConsole()) {
        printLastError(_T("FreeConsole failed with %d\n"));
        return 1;
    }

    if (!AllocConsole()) {
        printLastError(_T("AllocConsole failed with %d\n"));
        return 1;
    }

    g_hInputConsole = GetStdHandle(STD_INPUT_HANDLE);
    g_hOutputConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    if (g_hInputConsole == INVALID_HANDLE_VALUE || g_hInputConsole == NULL || g_hOutputConsole == INVALID_HANDLE_VALUE || g_hOutputConsole == NULL) {
        printLastError(_T("GetStdHandle failed with %d\n"));
        return 1;
    }

    SMALL_RECT windowRect = { 0, 0, CONSOLE_WIDTH - 1, CONSOLE_HEIGHT - 1 };
    if (!SetConsoleWindowInfo(g_hOutputConsole, TRUE, &windowRect)) {
        printLastError(_T("SetConsoleWindowInfo failed with %d\n"));
        return 1;
    }

    COORD bufferSize = { CONSOLE_WIDTH, CONSOLE_HEIGHT };
    if (!SetConsoleScreenBufferSize(g_hOutputConsole, bufferSize)) {
        printLastError(_T("SetConsoleScreenBufferSize failed with %d\n"));
        return 1;
    }

    g_hook = SetWinEventHook(
        EVENT_CONSOLE_CARET, // eventMin
        EVENT_CONSOLE_END_APPLICATION, //eventMax
        NULL, // hmodWinEventProc
        HandleWinEvent, // pfnWinEventProc
        0,
        0,
        WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS
    );
    if (!g_hook) {
        printLastError(_T("SetWinEventHook failed with %d\n"));
        return 1;
    }

    STARTUPINFO startupInfo;
    PROCESS_INFORMATION processInformation;

    ZeroMemory(&startupInfo, sizeof(startupInfo));
    startupInfo.cb = sizeof(startupInfo);
    ZeroMemory(&processInformation, sizeof(processInformation));

    // Determine full path of cmd.exe
    TCHAR cmdPath[MAX_PATH];
    if (FAILED(
        SHGetFolderPath(
            NULL,
            CSIDL_SYSTEM,
            NULL,
            0,
            cmdPath)
    )) {
        printLastError(_T("SHGetFolderPath failed with %d\n"));
        return 1;
    } else {
        if (FAILED(PathCchAppend(cmdPath, MAX_PATH, _T("cmd.exe")))) {
            printLastError(_T("PathCchAppend failed with %d\n"));
            return 1;
        }
    }

    if (!CreateProcess(
         cmdPath,
         NULL, // lpCommandLine
         NULL, // lpProcessAttributes
         NULL, // lpThreadAttributes
         FALSE, // bInheritHandles
         CREATE_NEW_PROCESS_GROUP,
         NULL, // lpEnvironment
         NULL, // lpCurrentDirectory
         &startupInfo,
         &processInformation
     )) {
        printLastError(_T("CreateProcess failed with %d\n"));
        return 1;
    }

    CloseHandle(processInformation.hThread);

    MSG msg;
    BOOL bRet;
    while ((bRet = GetMessage(&msg, NULL, 0, 0)) != 0)
    {
        if (bRet == -1) {
            printLastError(_T("GetMessage failed with %d\n"));
            return 1;
        }
        else {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    };

    UnhookWinEvent(g_hook);

    WaitForSingleObject(processInformation.hProcess, INFINITE);
    CloseHandle(processInformation.hProcess);

#if !_DEBUG
    CloseHandle(g_hFile);
#endif

    return 0;
}
