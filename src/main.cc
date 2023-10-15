/*
	em++ main.cc -std=c++20 -sUSE_SDL=2 -o web/sketch.html
*/

#include <emscripten.h>
#include <SDL2/SDL.h>
#include <iostream>
#include "window.hh"
#include "renderer.hh"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

struct AppState {
	bool quit = false;
	
	RawSketch example;
};

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

void draw(Window& w, Renderer& r, const AppState& s) {
	std::cout << "Rendering...\n";
	r.clear();
	r.displayRaw(s.example);
	w.updatePixels();
}

bool detectEvents(AppState& s) {
	bool input = false;
	for (SDL_Event ev; SDL_PollEvent(&ev); input=true)
		;
	return input;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

void appLoopBody(Window& w, Renderer& r, AppState& s) {
	if (detectEvents(s)) draw(w,r,s);
}

int main() {
	static // window gets destructed early if not set static.
	Window window {"Sketch Client", 800, 600};
	AppState state {};
	Renderer renderer {window.pixels, 800, 600,
		[=](Col3 c) -> uint32_t {
			return SDL_MapRGB(window.format, c.r, c.g, c.b);
		},
		[=](uint32_t pixel) -> Col3 {
			Col3 c;
			SDL_GetRGB(pixel, window.format, &c.r, &c.g, &c.b);
			return c;
		}
	};

	RawSketch example ({
		RawStroke ({{0,0}, {0,600}}),
		RawStroke ({{4,0}, {4,600}}),
		RawStroke ({{8,0}, {8,600}})//,
	});

	state.example = std::move(example);

#ifdef __EMSCRIPTEN__
	// Unsightly lambda expression syntax. This is the
	// best way I can think of passing in arguments to
	// appLoopBody while bottlenecked through a void*.
	std::tuple userData { &window, &renderer, &state };
	emscripten_set_main_loop_arg(
		[](void* data) {
			auto [w,r,s] = *(decltype(userData)*)data;
			appLoopBody(*w,*r,*s);
		},
		&userData,
		0, true
	);
#else
	while (!state.quit) appLoopBody(window, state);
#endif
}