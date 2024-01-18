#pragma once
// Header file for browser features we have to rely on
// Javascript for which SDL doesn't currently support.

namespace JS
{
	extern float penPressure;
	void listenForPenPressure();

	// XXX: keeps resetting to the empty string
	extern const char* clipboard;
	void copy();
	void paste();
};

extern "C"
{
	void jsSetPenPressure(float p);
	void jsSetClipboard(const char* str);
	const char* jsGetClipboard();
}