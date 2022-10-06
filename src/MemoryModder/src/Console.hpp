#pragma once

#include <Windows.h>
#include <Psapi.h>

#include <vector>
#include <sstream>

#include "Types.hpp"

// Windows.h missing functions
BOOL GetConsoleCursorPos(HANDLE handle, PCOORD coord) {
    CONSOLE_SCREEN_BUFFER_INFO cbsi;
    if(GetConsoleScreenBufferInfo(handle, &cbsi)) {
        coord->X = cbsi.dwCursorPosition.X;
        coord->Y = cbsi.dwCursorPosition.Y;
        return 1;
    }
    return 0;
}

BOOL GetConsoleTextAttribute(HANDLE handle, PWORD style) {
    CONSOLE_SCREEN_BUFFER_INFO cbsi;
    if(GetConsoleScreenBufferInfo(handle, &cbsi)) {
        *style = cbsi.wAttributes;
        return 1;
    }
    return 0;
}

enum struct ConsoleKey : UInt32 {
    Backspace = VK_BACK,
    Tab = VK_TAB,
    Return = VK_RETURN,
    Shift = VK_SHIFT,
    Control = VK_CONTROL,
    Alt = VK_MENU,
    Pause = VK_PAUSE,
    Caps = VK_CAPITAL,
    Escape = VK_ESCAPE,
    Space = VK_SPACE,
    PageUp = VK_PRIOR,
    PageDown = VK_NEXT,
    End = VK_END,
    Home = VK_HOME,
    ArrowLeft = VK_LEFT,
    ArrowUp = VK_UP,
    ArrowRight = VK_RIGHT,
    ArrowDown = VK_DOWN,
    Select = VK_SELECT,
    Print = VK_PRINT,
    PrintScreen = VK_SNAPSHOT,
    Insert = VK_INSERT,
    Delete = VK_DELETE,
    Help = VK_HELP,
    D0 = 0x30,
    D1,
    D2,
    D3,
    D4,
    D5,
    D6,
    D7,
    D8,
    D9,
    A = 0x41,
    B,
    C,
    D,
    E,
    F,
    G,
    H,
    I,
    J,
    K,
    L,
    M,
    N,
    O,
    P,
    Q,
    R,
    S,
    T,
    U,
    V,
    W,
    X,
    Y,
    Z,
    LeftWindows = VK_LWIN,
    RightWindows = VK_RWIN,
    Apps = VK_APPS,
    Sleep = VK_SLEEP,
    Numpad0 = VK_NUMPAD0,
    Numpad1 = VK_NUMPAD1,
    Numpad2 = VK_NUMPAD2,
    Numpad3 = VK_NUMPAD3,
    Numpad4 = VK_NUMPAD4,
    Numpad5 = VK_NUMPAD5,
    Numpad6 = VK_NUMPAD6,
    Numpad7 = VK_NUMPAD7,
    Numpad8 = VK_NUMPAD8,
    Numpad9 = VK_NUMPAD9,
    NumpadAdd = VK_ADD,
    NumpadSubtract = VK_SUBTRACT,
    NumpadMultiply = VK_MULTIPLY,
    NumpadDivide = VK_DIVIDE,
    NumpadDecimal = VK_DECIMAL,
    Numlock = VK_NUMLOCK,
    F1 = VK_F1,
    F2 = VK_F2,
    F3 = VK_F3,
    F4 = VK_F4,
    F5 = VK_F5,
    F6 = VK_F6,
    F7 = VK_F7,
    F8 = VK_F8,
    F9 = VK_F9,
    F10 = VK_F10,
    F11 = VK_F11,
    F12 = VK_F12,
    F13 = VK_F13,
    F14 = VK_F14,
    F15 = VK_F15,
    F16 = VK_F16,
    F17 = VK_F17,
    F18 = VK_F18,
    F19 = VK_F19,
    F20 = VK_F20,
    F21 = VK_F21,
    F22 = VK_F22,
    F23 = VK_F23,
    F24 = VK_F24,
    LeftShift = VK_LSHIFT,
    RightShift = VK_RSHIFT,
    LeftControl = VK_LCONTROL,
    RightControl = VK_RCONTROL,
    LeftAlt = VK_LMENU,
    RightAlt = VK_RMENU,

};

struct ConsoleKeyEvent {
public:
    Boolean down;
    ConsoleKey key;
};

struct Console {
public:
    static void Write(String const& string) {
        WriteConsoleA(_handleConsoleOutput, string.data(), static_cast<DWORD>(string.length()), nullptr, nullptr);
    }

    static void WriteLine(String const& string = "") {
        Write(string + "\n");
    }

    static String ReadLine() {
        int const MAX_SIZE = 2048;
        char* buf = new char[MAX_SIZE];
        DWORD sizeRead;

        ReadConsoleA(_handleConsoleInput, buf, MAX_SIZE, &sizeRead, nullptr);

        String string = String(buf, sizeRead - 2);

        delete[] buf;

        return string;
    }

    static void Clear() {
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        SMALL_RECT scrollRect;
        COORD scrollTarget;
        CHAR_INFO fill;

        // Get the number of character cells in the current buffer.
        if(!GetConsoleScreenBufferInfo(_handleConsoleOutput, &csbi)) {
            return;
        }

        // Scroll the rectangle of the entire buffer.
        scrollRect.Left = 0;
        scrollRect.Top = 0;
        scrollRect.Right = csbi.dwSize.X;
        scrollRect.Bottom = csbi.dwSize.Y;

        // Scroll it upwards off the top of the buffer with a magnitude of the entire height.
        scrollTarget.X = 0;
        scrollTarget.Y = (SHORT)(0 - csbi.dwSize.Y);

        // Fill with empty spaces with the buffer's default text attribute.
        fill.Char.UnicodeChar = TEXT(' ');
        fill.Attributes = csbi.wAttributes;

        // Do the scroll
        ScrollConsoleScreenBuffer(_handleConsoleOutput, &scrollRect, NULL, scrollTarget, &fill);

        // Move the cursor to the top left corner too.
        csbi.dwCursorPosition.X = 0;
        csbi.dwCursorPosition.Y = 0;

        SetConsoleCursorPosition(_handleConsoleOutput, csbi.dwCursorPosition);
    }
    
    static void WaitForInput(UInt32 timeout) {
        WaitForInputIdle(_handleConsoleOutput, timeout);
    }

    static std::vector<ConsoleKeyEvent> PollKeyEvents() {
        int const MAX_SIZE = 256;
        INPUT_RECORD* records = new INPUT_RECORD[MAX_SIZE];
        DWORD recordsRead = 0;
        
        PeekConsoleInputA(_handleConsoleInput, records, MAX_SIZE, &recordsRead);

        std::vector<ConsoleKeyEvent> keys = std::vector<ConsoleKeyEvent>();

        for(DWORD i = 0; i < recordsRead; ++i) {
            INPUT_RECORD const& record = records[i];
            if(record.EventType == KEY_EVENT) {
                keys.push_back(ConsoleKeyEvent(record.Event.KeyEvent.bKeyDown, static_cast<ConsoleKey>(record.Event.KeyEvent.wVirtualKeyCode)));
            }
        }

        delete[] records;

        return keys;
    }

    static std::vector<ConsoleKeyEvent> WaitKeyEvents() {
        int const MAX_SIZE = 256;
        INPUT_RECORD* records = new INPUT_RECORD[MAX_SIZE];
        DWORD recordsRead = 0;

        ReadConsoleInputA(_handleConsoleInput, records, MAX_SIZE, &recordsRead);

        std::vector<ConsoleKeyEvent> keys = std::vector<ConsoleKeyEvent>();

        for(DWORD i = 0; i < recordsRead; ++i) {
            INPUT_RECORD const& record = records[i];
            if(record.EventType == KEY_EVENT) {
                keys.push_back(ConsoleKeyEvent(record.Event.KeyEvent.bKeyDown, static_cast<ConsoleKey>(record.Event.KeyEvent.wVirtualKeyCode)));
            }
        }

        delete[] records;

        return keys;
    }

    static void WarningLine(String const& string) {
        UInt16 previousStyle = GetTextStyle();

        SetTextStyle(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
        WriteLine(string);

        SetTextStyle(previousStyle);

        Sleep(1000);
    }

    static void ErrorLine(String const& string) {
        UInt16 previousStyle = GetTextStyle();

        SetTextStyle(FOREGROUND_RED | FOREGROUND_INTENSITY);
        WriteLine(string);

        SetTextStyle(previousStyle);

        Sleep(1000);
    }

    static void SetTextColor(UInt8 red, UInt8 green, UInt8 blue) {
        WriteLine("\x1b[38;2;" + ToString<UInt8>(red) + ";" + ToString<UInt8>(green) + ";" + ToString<UInt8>(blue));
    }

    static void SetBackgroundColor(UInt8 red, UInt8 green, UInt8 blue) {
        WriteLine("\x1b[48;2;" + ToString<UInt8>(red) + ";" + ToString<UInt8>(green) + ";" + ToString<UInt8>(blue));
    }

    static void SetColors(UInt8 textRed, UInt8 textGreen, UInt8 textBlue, UInt8 backgroundRed, UInt8 backgroundGreen, UInt8 backgroundBlue) {
        SetTextColor(textRed, textGreen, textBlue);
        SetBackgroundColor(backgroundRed, backgroundGreen, backgroundBlue);
    }

    static void SetTextStyle(UInt16 style) {
        SetConsoleTextAttribute(_handleConsoleOutput, style);
    }

    static UInt16 GetTextStyle() {
        UInt16 style;
        GetConsoleTextAttribute(_handleConsoleOutput, &style);
        return style;
    }

    static void ResetTextStyle() {
        SetTextStyle(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    }

    static void GetPosition(Int32& x, Int32& y) {
        RECT r;
        GetWindowRect(_hwndConsole, &r);
        x = r.left;
        y = r.top;
    }

    static void SetPosition(Int32 px, Int32 py) {
        Int32 sx, sy;
        GetSize(sx, sy);
        MoveWindow(_hwndConsole, px, py, sx, sy, TRUE);
    }

    static void GetSize(Int32& x, Int32& y) {
        RECT r;
        GetWindowRect(_hwndConsole, &r);
        x = r.right - r.left;
        y = r.bottom - r.top;
    }

    static void SetSize(Int32 sx, Int32 sy) {
        Int32 px, py;
        GetPosition(px, py);
        MoveWindow(_hwndConsole, px, py, sx, sy, TRUE);
    }

    static void GetCursorPosition(Int16& x, Int16& y) {
        COORD coord;
        GetConsoleCursorPos(_handleConsoleOutput, &coord);
        x = coord.X;
        y = coord.Y;
    }

    static void SetCursorPosition(Int16 x, Int16 y) {
        COORD pos = COORD(x, y);
        SetConsoleCursorPosition(_handleConsoleOutput, pos);
    }

private:
    static HANDLE _handleConsoleOutput;
    static HANDLE _handleConsoleInput;
    static HWND _hwndConsole;
};

HANDLE Console::_handleConsoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);
HANDLE Console::_handleConsoleInput = GetStdHandle(STD_INPUT_HANDLE);
HWND Console::_hwndConsole = GetConsoleWindow();
