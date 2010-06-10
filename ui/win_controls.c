#include <commctrl.h>

#define ID_MENU         102
#define ID_FILE_EXIT    40001
#define ID_CONN_TOGGLE  40002

#define IDC_MAIN_TEXT 1
#define IDC_STATUS    2
#define IDC_INPUT     3
#define IDC_HOST      4

#define TEXTBOX_HEIGHT 28

static void toggle_conn_view(void);
static void controls_init(HWND);
static void controls_resize(HWND);

static char conn_view_visible = 0;

static void controls_init(HWND hwnd)
{
#define CREATE_EDIT(x, name, flags) \
			x = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "", flags | WS_CHILD | WS_VISIBLE , \
				0, 0, 100, TEXTBOX_HEIGHT, hwnd, (HMENU)name, GetModuleHandle(NULL), NULL); \
			SendMessage(x, WM_SETFONT, (WPARAM)hfdefault, MAKELPARAM(FALSE, 0));

	HFONT hfdefault = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	HWND hedit, hstatus, hinput, hhost;
	int statwidths[] = {100, -1};

	CREATE_EDIT(hedit,  IDC_MAIN_TEXT, WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL );
	CREATE_EDIT(hinput, IDC_INPUT,     0);
	CREATE_EDIT(hhost,  IDC_HOST,      0);

	hstatus = CreateWindowEx(0, STATUSCLASSNAME, NULL,
		WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP, 0, 0, 0, 0,
		hwnd, (HMENU)IDC_STATUS, GetModuleHandle(NULL), NULL);

	SendMessage(hstatus, SB_SETPARTS, sizeof(statwidths)/sizeof(statwidths[0]), (LPARAM)statwidths);
	SendMessage(hstatus, SB_SETTEXT, 0, (LPARAM)"Comm v2.0");
	SendMessage(hstatus, SB_SETTEXT, 1, (LPARAM)"Status: Jimmy");
}


static void controls_resize(HWND hwnd)
{
	HWND hstatus, hmain, hinput, hhost;
	RECT statusrect, clientrect, inputrect;
	int maintop, mainheight,
			inputtop, inputheight,
			statusheight;

	hmain   = GetDlgItem(hwnd, IDC_MAIN_TEXT);
	hinput  = GetDlgItem(hwnd, IDC_INPUT);
	hstatus = GetDlgItem(hwnd, IDC_STATUS);
	hhost   = GetDlgItem(hwnd, IDC_HOST);

	SendMessage(hstatus, WM_SIZE, 0, 0);

	GetClientRect(hwnd,    &clientrect);
	GetWindowRect(hstatus, &statusrect);
	GetWindowRect(hinput,  &inputrect);

	maintop      = 0;
	mainheight   = clientrect.bottom - statusheight - inputheight;

	inputtop     = mainheight;
	inputheight  = inputrect.bottom  - inputrect.top;

	statusheight = statusrect.bottom - statusrect.top;


	if(conn_view_visible){
		maintop += TEXTBOX_HEIGHT;
		mainheight -= TEXTBOX_HEIGHT;
		ShowWindow(hhost, SW_SHOWNORMAL);
	}else
		ShowWindow(hhost, SW_HIDE);

	SetWindowPos(hmain,  NULL, 0, inputtop, clientrect.right, mainheight,  0);
	SetWindowPos(hinput, NULL, 0,  mainheight, clientrect.right, inputheight, 0);
}

static void toggle_conn_view()
{
	conn_view_visible = 1 - conn_view_visible;
	controls_resize(mainhwnd);
}
