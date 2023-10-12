/*
	em++ main.cc -std=c++20 -sUSE_SDL=2 -o sketch.html
*/

#include <emscripten.h>
#include <SDL2/SDL.h>
#include <iostream>
#include "window.hh"
#include "renderer.hh"

int main() {
	Window window {"Parser Examples", 800, 600};
	Renderer renderer {window.pixels, 800, 600,
		[=](auto r, auto g, auto b) {
			return SDL_MapRGB(window.format, r,g,b);
		}
	};

	RawSketch example {
		RawStroke {{0,0}, {800,600}},
		RawStroke {{800,0}, {0,600}},
		RawStroke {{0,0}, {37,37}, {74,74}, {111,111}}
	};

	renderer.clear();
	renderer.displayRaw(example);

	window.updatePixels();
	emscripten_set_main_loop([]{}, 0, true);
}