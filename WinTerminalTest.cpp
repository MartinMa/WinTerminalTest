#include <iostream>
#include <windows.h>
#include <tchar.h>
#include <Shlobj_core.h>
#include <pathcch.h>

// Global constants.
const SHORT CONSOLE_HEIGHT = 25;
const SHORT CONSOLE_WIDTH = 80;

// Global variables.
HANDLE hInputConsole = NULL;
HANDLE hOutputConsole = NULL;
HWINEVENTHOOK g_hook;

void CALLBACK HandleWinEvent(HWINEVENTHOOK hook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime)
{
    switch (event) {
    case EVENT_CONSOLE_CARET:
        OutputDebugString(_T("EVENT_CONSOLE_CARET\n"));
        break;
    case EVENT_CONSOLE_UPDATE_REGION:
        OutputDebugString(_T("EVENT_CONSOLE_UPDATE_REGION\n"));
        break;
    case EVENT_CONSOLE_UPDATE_SIMPLE:
        OutputDebugString(_T("EVENT_CONSOLE_UPDATE_SIMPLE\n"));
        break;
    case EVENT_CONSOLE_UPDATE_SCROLL:
        OutputDebugString(_T("EVENT_CONSOLE_UPDATE_SCROLL\n"));
        break;
    case EVENT_CONSOLE_LAYOUT:
        OutputDebugString(_T("EVENT_CONSOLE_LAYOUT\n"));
        break;
    case EVENT_CONSOLE_START_APPLICATION:
        OutputDebugString(_T("EVENT_CONSOLE_START_APPLICATION\n"));
        break;
    case EVENT_CONSOLE_END_APPLICATION:
        OutputDebugString(_T("EVENT_CONSOLE_END_APPLICATION\n"));
        break;
    default:
        OutputDebugString(_T("UNKNOWN EVENT\n"));
        break;
    }
}

void printLastError(const TCHAR *format)
{
    TCHAR message[1024];
    _stprintf_s(message, 1024, format, GetLastError());
    OutputDebugString(message);
}

int main()
{
    std::cout << "Console Event Test!\n";

    if (!FreeConsole()) {
        printLastError(_T("FreeConsole failed with %d\n"));
        return 1;
    }

    if (!AllocConsole()) {
        printLastError(_T("AllocConsole failed with %d\n"));
        return 1;
    }

    hInputConsole = GetStdHandle(STD_INPUT_HANDLE);
    hOutputConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    if (hInputConsole == INVALID_HANDLE_VALUE || hInputConsole == NULL || hOutputConsole == INVALID_HANDLE_VALUE || hOutputConsole == NULL) {
        printLastError(_T("GetStdHandle failed with %d\n"));
        return 1;
    }

    SMALL_RECT windowRect = { 0, 0, CONSOLE_WIDTH - 1, CONSOLE_HEIGHT - 1 };
    if (!SetConsoleWindowInfo(hOutputConsole, TRUE, &windowRect)) {
        printLastError(_T("SetConsoleWindowInfo failed with %d\n"));
        return 1;
    }

    COORD bufferSize = { CONSOLE_WIDTH, CONSOLE_HEIGHT };
    if (!SetConsoleScreenBufferSize(hOutputConsole, bufferSize)) {
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

    return 0;
}
