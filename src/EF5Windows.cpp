#include <windows.h>

// Other headers
#include <cstdio>
#include <process.h>
#include <richedit.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>


// User headers
#include "EF5Windows.h"
#include "Config.h"
#include "Defines.h"
#include "ExecutionController.h"

#define COLOR_WHITE 0xFFFFFF
#define COLOR_GREY 0xC0C0C0
#define COLOR_BLUE 0x734D26
#define COLOR_GREEN 0x4D7326
#define COLOR_RED 0x262673
#define COLOR_YELLOW 0x267373
#define COLOR_PURPLE 0x732673
#define COLOR_BLUEGREEN 0x737326
#define COLOR_MIDBLUE 0xA16B36
#define COLOR_DARKBLUE 0x732626
#define COLOR_DARKGREEN 0x267326
#define COLOR_DARKYELLOW 0x264D73
#define COLOR_DARKRED 0x4D2673
#define COLOR_DARKPURPLE 0x73264D
#define COLOR_LIGHTBLUE 0xC48A4F

int CreateWindows(HINSTANCE hInstance);
void DestroyWindows();
void AddText(const char *szFmt, ...);
void SetColor(COLORREF Color);
void TimeStamp();

extern Config *g_config;

static LRESULT CALLBACK MainWndProc(HWND hWnd, UINT message, WPARAM wParam,
                                    LPARAM lParam);
static LRESULT CALLBACK InfoSubclass(HWND Edit, UINT uMsg, WPARAM wParam,
                                     LPARAM lParam);
static void PrintStartupMessage();
static void threadProc(PVOID params);
static DWORD CALLBACK editStreamCallback(DWORD_PTR dwCookie, LPBYTE pbBuff,
                                         LONG cb, LONG *pcb);

HANDLE hThread = NULL;
HWND hMainWindow = NULL, hInfo = NULL, hTime = NULL;
WNDCLASSEX wcex;
WNDPROC wpInfo = NULL;
HINSTANCE hREDLL = NULL;
COLORREF CColor = 0;
HINSTANCE hProgram = NULL;
char szTxtToAppend[4096] = "";
size_t textToAppendSize = 0;
unsigned long sizeOfTextToAppend = 0;
static void MsgLoop();
int g_iStopLoop = 0;
extern HANDLE hWaitObject;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpszArgument, int nFunsterStil) {
  int result = EF5_ERROR_SUCCESS;
  if (!CreateWindows(hInstance)) {
    MessageBox(0, "Failed to create the needed windows.", "EF5", MB_ICONERROR);
    return EF5_ERROR_INVALIDCONF;
  }
  PrintStartupMessage();
  _putenv("TZ=UTC");
  _tzset();
  g_config = new Config((lpszArgument[0]) ? lpszArgument : "control.txt");
  if (g_config->ParseConfig() != CONFIG_SUCCESS) {
    result = EF5_ERROR_INVALIDCONF;
  } else {
    hThread = (HANDLE)_beginthread(threadProc, 0, NULL);
  }
  MsgLoop();
  DestroyWindows();
  (void)hPrevInstance;
  (void)lpszArgument;
  (void)nFunsterStil;
  return result;
}

static void MsgLoop() {
  while (!g_iStopLoop) {
    if (MsgWaitForMultipleObjectsEx(0, 0, INFINITE, QS_ALLINPUT,
                                    MWMO_ALERTABLE) == WAIT_OBJECT_0) {
      MSG msg;
      while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
      }
    }
  }
}

int CreateWindows(HINSTANCE hInstance) {
  PARAFORMAT2 pf;
  CHARFORMAT cfFormat;
  hREDLL = LoadLibrary("RichEd32.dll");
  if (!hREDLL)
    return 0;
  hProgram = hInstance;
  wcex.cbSize = sizeof(WNDCLASSEX);
  wcex.style = CS_HREDRAW | CS_VREDRAW;
  wcex.lpfnWndProc = (WNDPROC)MainWndProc;
  wcex.cbClsExtra = 0;
  wcex.cbWndExtra = 0;
  wcex.hInstance = hInstance;
  wcex.hIcon = NULL; // LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON));
  wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
  wcex.hbrBackground = (HBRUSH)COLOR_BACKGROUND;
  wcex.lpszMenuName = NULL;
  wcex.lpszClassName = "EF5Class";
  wcex.hIconSm = NULL; // LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON));
  if (!RegisterClassEx(&wcex)) {
    FreeLibrary(hREDLL);
    return 0;
  }
  hMainWindow = CreateWindowEx(
      WS_EX_APPWINDOW | WS_EX_OVERLAPPEDWINDOW, "EF5Class",
      "Ensemble Framework For Flash Flood Forecasting",
      WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, CW_USEDEFAULT, CW_USEDEFAULT,
      CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInstance, NULL);
  if (!hMainWindow) {
    UnregisterClass("EF5Class", hInstance);
    FreeLibrary(hREDLL);
    return 0;
  }
  hInfo = CreateWindowEx(0, RICHEDIT_CLASS, "",
                         WS_CHILD | WS_VSCROLL | ES_AUTOVSCROLL | ES_MULTILINE |
                             ES_NOHIDESEL | ES_READONLY,
                         20, 20, 290, 120, hMainWindow, NULL, hInstance, NULL);
  if (!hInfo) {
    DestroyWindow(hMainWindow);
    UnregisterClass("EF5Class", hInstance);
    FreeLibrary(hREDLL);
    return 0;
  }
  hTime = CreateWindowEx(WS_EX_TRANSPARENT, "STATIC", "", WS_CHILD, 20, 20, 290,
                         120, hMainWindow, NULL, hInstance, NULL);
  if (!hTime) {
    DestroyWindow(hMainWindow);
    UnregisterClass("EF5Class", hInstance);
    FreeLibrary(hREDLL);
    return 0;
  }
  SendMessage(hTime, WM_SETTEXT, NULL, (LPARAM) "Current Timestep: ");

  // hSend = CreateWindowEx(0, RICHEDIT_CLASS, "", WS_CHILD  , 20, 180, 290, 30,
  // hMainWindow, NULL, hInstance, NULL);  if (!hSend) {
  //		return 0;
  //	}
  pf.cbSize = sizeof(PARAFORMAT2);
  SendMessage(hInfo, EM_GETPARAFORMAT, 0, (LPARAM)&pf);
  pf.dwMask = PFM_OFFSET;
  pf.dxOffset = 1120;
  SendMessage(hInfo, EM_SETPARAFORMAT, 0, (LPARAM)&pf);
  SendMessage(hInfo, EM_AUTOURLDETECT, TRUE, 0);
  SendMessage(hInfo, EM_SETEVENTMASK, 0, ENM_LINK);
  SendMessage(hInfo, EM_SETBKGNDCOLOR, FALSE, 0x00000000);
  //	SendMessage(hSend, EM_SETBKGNDCOLOR, FALSE, 0x00000000);
  cfFormat.cbSize = sizeof(CHARFORMAT);
  cfFormat.dwMask = CFM_COLOR | CFM_FACE | CFM_SIZE;
  cfFormat.dwEffects = CFE_PROTECTED;
  cfFormat.yHeight = 190;
  cfFormat.yOffset = 0;
  cfFormat.crTextColor = COLOR_WHITE;
  cfFormat.bCharSet = DEFAULT_CHARSET;
  cfFormat.bPitchAndFamily = DEFAULT_PITCH;
  strncpy(cfFormat.szFaceName, "Tahoma", 31);
  cfFormat.szFaceName[31] = 0;
  //	SendMessage(hSend, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&cfFormat);
  //	SendMessage(hSend, EM_EXLIMITTEXT, (WPARAM)0, (LPARAM)200);
  wpInfo = (WNDPROC)(LONG_PTR)SetWindowLong(hInfo, GWLP_WNDPROC,
                                            (LONG)(LONG_PTR)InfoSubclass);
  //	wpSend = (WNDPROC)(LONG_PTR)SetWindowLong(hSend, GWL_WNDPROC,
  //(LONG)(LONG_PTR)SendSubclass);
  ShowWindow(hMainWindow, SW_SHOWNORMAL);
  ShowWindow(hInfo, SW_SHOWNORMAL);
  ShowWindow(hTime, SW_SHOWNORMAL);
  return 1;
}

void DestroyWindows() {
  SetWindowLong(hInfo, GWLP_WNDPROC, (LONG)(LONG_PTR)wpInfo);
  // SetWindowLong(hSend, GWL_WNDPROC, (LONG)(LONG_PTR)wpSend);
  DestroyWindow(hMainWindow); // Children get killed automatically
  UnregisterClass("EF5Class", hProgram);
  FreeLibrary(hREDLL);
}

static LRESULT CALLBACK MainWndProc(HWND hWnd, UINT message, WPARAM wParam,
                                    LPARAM lParam) {
  switch (message) {
  case WM_CLOSE: {
    g_iStopLoop = 1;
    return 0;
  }
  case WM_SIZE: {
    RECT mainw = {0}, infow = {0};
    GetClientRect(hMainWindow, &mainw);
    SetWindowPos(hInfo, NULL, 20, 20, mainw.right - 40, mainw.bottom - 40,
                 SWP_NOACTIVATE | SWP_NOZORDER);
    SetWindowPos(hTime, NULL, 20, 3, mainw.right - 40, 15,
                 SWP_NOACTIVATE | SWP_NOZORDER);
    return 0;
  }
  case WM_CREATE: {
    return 0;
  }
  default: { return DefWindowProc(hWnd, message, wParam, lParam); }
  }
}

static LRESULT CALLBACK InfoSubclass(HWND Edit, UINT uMsg, WPARAM wParam,
                                     LPARAM lParam) {
  switch (uMsg) {
  case WM_KEYDOWN:
  case WM_CHAR: {
    if (wParam == 'C' && (GetKeyState(VK_CONTROL) & 0x8000))
      return CallWindowProc(wpInfo, Edit, uMsg, wParam, lParam);
    return 0;
  }
  default: { return CallWindowProc(wpInfo, Edit, uMsg, wParam, lParam); }
  }
}

void AddText(const char *szFmt, ...) {
  LONG linecount2 = 0; //, linecount2 = 0;
  SCROLLINFO SI;
  CHARFORMAT cfFormat;
  POINT p;
  CHARRANGE Range = {-1, -1}, Range2 = {-1, -1}, Range3 = {-1, -1};
  EDITSTREAM editStream;
  va_list vaArg;
  va_start(vaArg, szFmt);
  _vsnprintf(szTxtToAppend, 4096, szFmt, vaArg);
  va_end(vaArg);
  szTxtToAppend[4095] = 0;
  textToAppendSize = strlen(szTxtToAppend);
  // if (nStopLoop == 1)
  //	return;
  ZeroMemory(&SI, sizeof(SI));
  SI.cbSize = sizeof(SI);
  SI.fMask = 7;
  cfFormat.cbSize = sizeof(CHARFORMAT);
  cfFormat.dwMask = CFM_COLOR | CFM_FACE | CFM_SIZE | CFM_BOLD;
  cfFormat.dwEffects = CFE_PROTECTED;
  cfFormat.yHeight = 190;
  cfFormat.yOffset = 0;
  cfFormat.crTextColor = CColor;
  cfFormat.bCharSet = DEFAULT_CHARSET;
  cfFormat.bPitchAndFamily = DEFAULT_PITCH;
  strncpy(cfFormat.szFaceName, "Tahoma", 32);
  cfFormat.szFaceName[31] = 0;
  SendMessage(hInfo, WM_SETREDRAW, 0, 0);
  SendMessage(hInfo, EM_GETSCROLLPOS, 0, (LPARAM)&p);
  GetScrollInfo(hInfo, SB_VERT, &SI);
  // linecount = (LONG)SendMessage(hInfo, EM_GETLINECOUNT, 0, 0);
  SendMessage(hInfo, EM_EXGETSEL, 0, (LPARAM)&Range);
  SendMessage(hInfo, EM_EXSETSEL, 0, (LPARAM)&Range2);
  SendMessage(hInfo, EM_EXGETSEL, 0, (LPARAM)&Range3);
  SendMessage(hInfo, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cfFormat);
  editStream.dwCookie = 0;
  editStream.dwError = 0;
  editStream.pfnCallback = editStreamCallback;
  SendMessage(hInfo, EM_STREAMIN, SF_TEXT | SFF_SELECTION, (LPARAM)&editStream);
  linecount2 = (LONG)SendMessage(hInfo, EM_GETLINECOUNT, 0, 0);
  if (linecount2 > 2000) {
    Range2.cpMin = 0;
    Range2.cpMax = (LONG)SendMessage(hInfo, EM_LINEINDEX, 1, 0);
    SendMessage(hInfo, EM_EXSETSEL, 0, (LPARAM)&Range2);
    SendMessage(hInfo, EM_REPLACESEL, 0, (LPARAM) "");
    Range.cpMin -= Range2.cpMax;
    Range.cpMax -= Range2.cpMax;
  }
  SendMessage(hInfo, EM_EXSETSEL, 0, (LPARAM)&Range);
  SendMessage(hInfo, EM_SETSCROLLPOS, 0, (LPARAM)&p);

  // if(linecount == 1 || SI.nMax == SI.nPos + (int)SI.nPage) {
  SendMessage(hInfo, WM_VSCROLL, SB_BOTTOM, 0);
  //} else if (linecount2 > 2000) {
  //	SendMessage(hInfo, EM_LINESCROLL, 0, -1);
  //}
  SendMessage(hInfo, WM_SETREDRAW, 1, 0);
  InvalidateRect(hInfo, 0, 1);
}

static DWORD CALLBACK editStreamCallback(DWORD_PTR dwCookie, LPBYTE pbBuff,
                                         LONG cb, LONG *pcb) {
  *pcb = cb;
  if (*pcb > (LONG)textToAppendSize) {
    *pcb = (LONG)textToAppendSize;
  }
  if (*pcb > 0) {
    memcpy(pbBuff, szTxtToAppend, *pcb);
    textToAppendSize -= *pcb;
  }
  return 0;
  (void)dwCookie;
}

void SetColor(COLORREF Color) { CColor = Color; }

void TimeStamp() {
  SYSTEMTIME st;
  GetLocalTime(&st);
  SetColor(COLOR_WHITE);
  AddText("[%02i:%02i:%02i] ", st.wHour, st.wMinute, st.wSecond);
}

void addConsoleText(CONSOLEMESSAGETYPE type, const char *szFmt, ...) {
  char txtToAppend[4096] = "";
  va_list vaArg;
  va_start(vaArg, szFmt);
  _vsnprintf(txtToAppend, 4096, szFmt, vaArg);
  va_end(vaArg);
  szTxtToAppend[4095] = 0;
  // TimeStamp();
  switch (type) {
  case NORMAL:
    SetColor(COLOR_LIGHTBLUE);
    AddText("%s", txtToAppend);
    break;
  case INFORMATION:
    SetColor(COLOR_GREEN);
    AddText("%s", "INFO: ");
    SetColor(COLOR_WHITE);
    AddText("%s\n", txtToAppend);
    break;
  case WARNING:
    SetColor(COLOR_YELLOW);
    AddText("%s", "WARNING: ");
    SetColor(COLOR_WHITE);
    AddText("%s\n", txtToAppend);
    break;
  case FATAL:
    SetColor(COLOR_RED);
    AddText("%s", "ERROR: ");
    SetColor(COLOR_WHITE);
    AddText("%s\n", txtToAppend);
    break;
  }
}

void setTimestep(const char *szTime) {
  char txt[4096] = "";
  sprintf(txt, "Current Timestep: %s", szTime);
  SendMessage(hTime, WM_SETTEXT, NULL, (LPARAM)txt);
}

void setIteration(int current) {
  char txt[4096] = "";
  sprintf(txt, "Current Iteration: %i", current);
  SendMessage(hTime, WM_SETTEXT, NULL, (LPARAM)txt);
}

/*int main(int argc, char *argv[]) {

        PrintStartupMessage();

        g_config = new Config((argc == 2) ? argv[1] : "control.txt");
        if (g_config->ParseConfig() != CONFIG_SUCCESS) {
                return 1;
        }

        ExecuteTasks();

        return ERROR_SUCCESS;
}*/

void PrintStartupMessage() {
  addConsoleText(NORMAL, "%s",
                 "********************************************************\n");
  addConsoleText(NORMAL, "%s",
                 "**   Ensemble Framework For Flash Flood Forecasting   **\n");
  addConsoleText(NORMAL,
                 "**                   Version %s                     **\n",
                 EF5_VERSION);
  addConsoleText(NORMAL, "%s",
                 "********************************************************\n");
}

static void threadProc(PVOID params) {
  _putenv("TZ=UTC");
  _tzset();
  ExecuteTasks();
  addConsoleText(NORMAL, "%s", "Done!\n");
}
