# WinTerminalTest

This project is a proof-of-concept on how to observe changes to a screen buffer of a Windows terminal the "legacy way".

This program opens a new terminal window at 80 by 25 (columns by rows),
listens for console events and logs the data to the debugger output window of Visual Studio.

# Context
The minimum supported client for Windows Pseudo Console (ConPTY) is Windows 10 October 2018 Update (version 1809),
which offers a modern way to create a pseudo console and making input and output streams easily available.

On older Windows systems like Windows 10 LTSB / LTSC 1607 you still have to rely on classical console api functionality.
