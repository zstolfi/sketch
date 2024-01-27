#include "external.hh"
#include <emscripten.h>
#include <iostream>

float JS::penPressure = 1.0;
std::string_view JS::clipboard = "";

// https://discourse.libsdl.org/t/get-tablet-stylus-pressure/35319/2
void JS::listenForPenPressure() {
	EM_ASM (
		const jsSetPenPressure = Module.cwrap("jsSetPenPressure", "", ["number"]);
		Module.canvas.addEventListener("pointerdown", (ev) => jsSetPenPressure(ev.pressure));
		Module.canvas.addEventListener("pointermove", (ev) => jsSetPenPressure(ev.pressure));
		Module.canvas.addEventListener("pointerup"  , (ev) => jsSetPenPressure(ev.pressure));
	);
}

void JS::copy() {
	EM_ASM (
		const jsGetClipboard = Module.cwrap("jsGetClipboard", "string", [""]);
		navigator.clipboard.writeText(jsGetClipboard());
	);
}

void JS::paste() {
	EM_ASM (
		const jsSetClipboard = Module.cwrap("jsSetClipboard", "", ["string"]);
		navigator.clipboard.readText()
		.then((text) => {
			jsSetClipboard(text);
		})
		.catch((error) => {
			/* ... */
		});
	);
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

void jsSetPenPressure(float p) {
	JS::penPressure = p;
}

void jsSetClipboard(const char* str) {
	std::cout << "clipbaord set to \"" << str << "\"\n";
	JS::clipboard = str;
}

const char* jsGetClipboard() {
	std::cout << "\"" << JS::clipboard << "\" read from clipboard.\n";
	return JS::clipboard.data();
}