#include <stdafx.h>
#include <sapi.h>
#include <wchar.h>

/* http://msdn.microsoft.com/en-us/library/ms720163(VS.85).aspx */
#include "sapi.h"

static ISpVoice * pVoice = NULL;

int sapi_init()
{
	HRESULT hr;

	if(CoInitialize(NULL))
		return 1;

	hr = CoCreateInstance(CLSID_SpVoice, NULL, CLSCTX_ALL, IID_ISpVoice, (void **)&pVoice;);
	return SUCCEEDED(hr);
}

void sapi_speak(const char *s)
{
	const wchar_t *ws = malloc((strlen(s) + 1) * sizeof *ws);

	if(ws){
		if(mbstowcs(ws, s, strlen(s)) == (size_t -1)){
			hr = pVoice->Speak(ws, 0, NULL);
			free(ws);

			/* Change pitch */
			/*hr = pVoice->Speak(L"This sounds normal <pitch middle = '-10'/> but the pitch drops half way through", SPF_IS_XML, NULL);*/
		}else
			perror("mbstowcs()");
	}else
		perror("malloc()");
}

void sapi_term()
{
	pVoice->Release();
	pVoice = NULL;
	CoUninitialize();
}

