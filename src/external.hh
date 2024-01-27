#pragma once
#include <string_view>

// Header file for browser features that Javascript
// supplies. (Which SDL doesn't currently support).

namespace JS
{
	extern float penPressure;
	void listenForPenPressure();

	// TODO:
	extern bool mouseInFocus;
	void listenForMouseFocus();

	// TODO: keeps resetting to the empty string
	extern std::string_view clipboard;
	void copy();
	void paste();
};

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

extern "C"
{
	void jsSetPenPressure(float);
	void jsSetMouseFocus(bool);
	void jsSetClipboard(const char*);
	const char* jsGetClipboard();
}