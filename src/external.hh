#pragma once
// Header file for browser features we have to rely on
// Javascript for which SDL doesn't currently support.

namespace JS
{
	extern float penPressure;
	void listenForPenPressure();

	// TODO:
	extern bool mouseInFocus;
	void listenForMouseFocus();

	// XXX: keeps resetting to the empty string
	extern const char* clipboard;
	void copy();
	void paste();
};

extern "C"
{
	void jsSetPenPressure(float);
	void jsSetMouseFocus(bool);
	void jsSetClipboard(const char*);
	const char* jsGetClipboard();
}