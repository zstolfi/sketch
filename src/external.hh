#pragma once
// Header file for browser features we have to rely on
// Javascript for which SDL doesn't currently support.

static float jsPenPressure = 1.0;
extern "C" {
	void jsSetPenPressure(float p) {
		jsPenPressure = p;
	}
}

// https://discourse.libsdl.org/t/get-tablet-stylus-pressure/35319/2
void jsListenForPenPressure() {
	EM_ASM(
		const jsSetPenPressure = Module.cwrap("jsSetPenPressure", "", ["number"]);
		Module.canvas.addEventListener("pointerdown", (ev) => jsSetPenPressure(ev.pressure));
		Module.canvas.addEventListener("pointermove", (ev) => jsSetPenPressure(ev.pressure));
		Module.canvas.addEventListener("pointerup"  , (ev) => jsSetPenPressure(ev.pressure));
	);
}