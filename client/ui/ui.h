#ifndef UI_H
#define UI_H

enum ui_message_type
{
	UI_OK, UI_YESNO, UI_YESNOCANCEL, UI_ERROR
};

enum ui_answer
{
	UI_YES, UI_NO, UI_CANCEL
}	ui_message(enum ui_message_type, const char *);

char *ui_getstring(const char *);

void ui_addtext(const char *, ...);
int ui_main(const char *, char *, int);

#endif

