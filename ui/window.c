#include <windows.h>

#define WINDOW_CLASS "Comm v2.0"
#define WINDOW_TITLE WINDOW_CLASS

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// When using the windows API you use WinMain instead of main
/* int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrev, LPSTR lpCmdLine, int nShowCmd) */
int window_main(void)
{
	HWND hWnd = setupwindow();
	if(!hWnd)
		return 0;

	ShowWindow(hWnd, nShowCmd);

	UpdateWindow(hWnd);

	while(GetMessage(&msg, NULL, 0, 0) > 0){
		/* Convert virtual key messages into a char */
		TranslateMessage(&msg);
		/* Send the message to our WndProc function */
		DispatchMessage(&msg);
	}

	return msg.wParam;
}

// This will handle the messages.
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg){
		case WM_CLOSE:
			/* Tell the program to end, and dispath WM_DESTROY */
			DestroyWindow(hWnd);
			break;

		case WM_DESTROY:
			/* End the program. */
			PostQuitMessage(0);
			break;

		default:
			return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	return 0;
}

HWND setupwindow(void)
{
	WNDCLASSEX wc;
	HWND hWnd;
	MSG msg;
	HINSTANCE hInstance = GetModuleHandle(NULL);

	wc.cbSize		 = sizeof(WNDCLASSEX);
	wc.cbClsExtra	 = 0;
	wc.cbWndExtra	 = 0;
	wc.hbrBackground	= (HBRUSH)(COLOR_WINDOW);
	wc.hCursor		= LoadCursor(hInstance, IDC_ARROW);
	wc.hIcon			= LoadIcon(hInstance, IDI_APPLICATION);
	wc.hIconSm		= LoadIcon(hInstance, IDI_APPLICATION);
	wc.hInstance		= hInstance;
	wc.lpfnWndProc	= WndProc;
	wc.lpszClassName	= ClassName;
	wc.lpszMenuName	 = NULL;
	wc.style			= 0;

	if(!RegisterClassEx(&wc)){
		MessageBox(NULL, "Could not register window", "Error", MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}

	// Create the window using the "MyWindow" class.
	hWnd = CreateWindowEx(WS_EX_WINDOWEDGE, WINDOW_CLASS, WINDOW_TITLE,
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 200, 150,
		NULL, NULL, hInstance, NULL);

	// If the window was not created show error and exit.
	if(hWnd == NULL){
		MessageBox(NULL, "Could not create window", "Error", MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}

	return hWnd;
}
