/*
	em++ main.cc -std=c++20 -sUSE_SDL=2 -o sketch.html
*/

#include <emscripten.h>
#include <SDL2/SDL.h>
#include <iostream>
#include "window.hh"
#include "renderer.hh"

struct AppState {
	bool quit = false;
	
	RawSketch example;
};

void draw(Window& w, Renderer& r, const AppState& s) {
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

void appLoopBody(Window& w, Renderer& r, AppState& s) {
	if (detectEvents(s)) draw(w,r,s);
}

int main() {
	static // window gets destructed early if not set static.
	Window window {"Sketch Client", 800, 600};
	AppState state {};
	Renderer renderer {window.pixels, 800, 600,
		[=](auto r, auto g, auto b) {
			return SDL_MapRGB(window.format, r,g,b);
		},
	};

	RawSketch example ({
		RawStroke ({{0,0}, {800,600}}),
		RawStroke ({{800,0}, {0,600}}),
		RawStroke ({{0,0}, {37,37}, {74,74}, {111,111}})
	});

	state.example = std::move(example);

#ifdef __EMSCRIPTEN__
	// Unsightly lambda expression syntax... Basically
	// we want to pass in 'window' and 'state' as args
	// into the appLoopBody function with emscripten's
	// set_main function, but we're only allowed to do
	// so using a void*. I used a tuple<win*,state*>*.
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