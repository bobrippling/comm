#include <stdio.h>
#include <stdarg.h>
#include <windows.h>
#include <winsock2.h>

#include "ui.h"
#include "../log.h"
#include "../settings.h"
#include "../comm.h"

#define HEIGHT 400
#define WIDTH  (HEIGHT * 5/3)

#define window_title window_class
static const char *window_class = "Comm v2.0";
static HWND mainhwnd = NULL;
static int sock = -1;


#include "win_controls.c"


extern struct settings global_settings;

LRESULT CALLBACK wndproc(HWND, UINT, WPARAM, LPARAM);
static int ui_init(void);

enum ui_answer ui_message(enum ui_message_type t, const char *s)
{
	/* hwnd, text, caption, type */
	int mode, ret;

	switch(t){
		default:
		case UI_OK:
			mode = MB_OK | MB_ICONINFORMATION;
			break;
		case UI_YESNO:
			mode = MB_YESNO | MB_ICONQUESTION;
			break;
		case UI_YESNOCANCEL:
			mode = MB_YESNOCANCEL | MB_ICONQUESTION;
			break;
	}
	/*
	 * MB_ABORTRETRYIGNORE
	 * MB_OK
	 * MB_OKCANCEL
	 * MB_RETRYCANCEL
	 * MB_YESNO
	 * MB_YESNOCANCEL
	 *
	 * MB_ICONEXCLAMATION
	 * MB_ICONWARNING
	 * MB_ICONINFORMATION
	 * MB_ICONQUESTION
	 * MB_ICONSTOP
	 * MB_ICONERROR
	 */
	ret = MessageBox(mainhwnd, s, window_title, mode);

	switch(ret){
		case IDOK:
		case IDYES:
			return UI_YES;
		case IDNO:
			return UI_NO;
		case IDCANCEL:
		default:
			return UI_CANCEL;
	}
}

char *ui_getstring(const char *s)
{
	return NULL;
}

void ui_addtext(const char *s, ...)
{
	va_list l;

	va_start(l, s);
	/* TODO */
	va_end(l);
}

static int ui_init(void)
{
	WNDCLASSEX wc;
	HINSTANCE hinstance = GetModuleHandle(NULL);
	HMENU hmenu, hsubmenu;

	wc.cbSize        = sizeof(WNDCLASSEX);
	wc.style         = 0;
	wc.lpfnWndProc   = &wndproc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = hinstance;
	wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wc.lpszMenuName  = MAKEINTRESOURCE(ID_MENU);
	wc.lpszClassName = window_class;
  wc.hIconSm       = LoadIcon(NULL, IDI_APPLICATION);

	if(!RegisterClassEx(&wc)){
		MessageBox(NULL, "Failed to register window class", "Error" , MB_ICONEXCLAMATION | MB_OK);
		return 1;
	}

	mainhwnd = CreateWindowEx(WS_EX_CLIENTEDGE, window_class, window_title, WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT, CW_USEDEFAULT, WIDTH, HEIGHT, NULL, NULL, hinstance, NULL);

	if(mainhwnd == NULL){
	 MessageBox(NULL , "Failed to create window", "Error", MB_ICONEXCLAMATION | MB_OK);
	 return 1;
	}

	hmenu = CreateMenu();

	hsubmenu = CreatePopupMenu();
	AppendMenu(hsubmenu, MF_STRING, ID_FILE_EXIT, "E&xit");
	AppendMenu(hmenu,    MF_STRING | MF_POPUP, (unsigned int)hsubmenu, "&File");

	hsubmenu = CreatePopupMenu();
	AppendMenu(hsubmenu, MF_STRING, ID_CONN_TOGGLE, "&Show Dialogue");
	AppendMenu(hmenu,    MF_STRING | MF_POPUP, (unsigned int)hsubmenu, "&Connection");

	SetMenu(mainhwnd, hmenu);



	ShowWindow(mainhwnd, SW_SHOWNORMAL);
	/* SW_SHOWNORMAL | SW_SHOWMAXIMIZED | SW_SHOWMINIMIZED */

	UpdateWindow(mainhwnd);

	return 0;
}

LRESULT CALLBACK wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch(msg){
		case WM_CREATE:
			controls_init(hwnd);
			break;

		case WM_SIZE:
			controls_resize(hwnd);
			break;

		case WM_CLOSE:
			DestroyWindow(hwnd);
			break;

		case WM_DESTROY:
			PostQuitMessage(0);
			break;

		case WM_COMMAND:
			switch(LOWORD(wparam)){
				case ID_FILE_EXIT:
					PostMessage(hwnd, WM_CLOSE, 0, 0);
					break;

				case ID_CONN_TOGGLE:
					toggle_conn_view();
					break;
			}
			break;


		default:
			return DefWindowProc(hwnd, msg, wparam, lparam);
	}

	return 0;
}

int ui_main(void)
{
	MSG msg;
	WSADATA wsabs;

	if(WSAStartup(MAKEWORD(2,2), &wsabs)){
		perror("WSAStartup()");
		return 1;
	}

	if(ui_init()){
		ui_message(UI_ERROR, "Couldn't ui_init() failed");
		return 1;
	}

	while(GetMessage(&msg, NULL, 0, 0)){
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	WSACleanup();
	return 0;
}

