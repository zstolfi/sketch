/*
	em++ main.cc -std=c++20 -sUSE_SDL=2 -sEXPORTED_FUNCTIONS=_main,_jsSetPenPressure,_jsSetClipboard,_jsGetClipboard -sEXPORTED_RUNTIME_METHODS=cwrap --embed-file ../web/input@/ -o ../web/output/sketch.html
*/

#include <emscripten.h>
#include <SDL2/SDL.h>
#include <iostream>
#include <fstream>
#include "window.hh"
#include "external.hh"
#include "renderer.hh"
#include "parser.hh"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

struct AppState {
	bool quit = false;
	
	RawSketch example;
	Point cursor;
	bool pressed = false;
};

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

void draw(Window& w, Renderer& r, const AppState& s) {
	r.clear();
	r.displayRaw(s.example);
	w.updatePixels();
}

bool detectEvents(AppState& s) {
	bool input = false;
	for (SDL_Event ev; SDL_PollEvent(&ev); input=true)
	switch (ev.type) {
		case SDL_QUIT:
			s.quit = true;
			break;
		case SDL_MOUSEMOTION:
			s.cursor = {
				(int16_t) ev.motion.x,
				(int16_t) ev.motion.y,
				JS::penPressure
			};
			break;
		case SDL_MOUSEBUTTONDOWN:
			s.pressed = true;
			s.cursor.pressure = JS::penPressure;
			break;
		case SDL_MOUSEBUTTONUP:
			s.pressed = false;
			s.cursor.pressure = 0.0;
			break;
		case SDL_KEYDOWN:
			switch (ev.key.keysym.sym) {
				case SDLK_ESCAPE:
					s.quit = true;
					break;
				case SDLK_c: // TODO: figure out modifier
					JS::copy();
					break;
				case SDLK_v:
					JS::paste();
					break;
			} break;
	}
	return input;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

void appLoopBody(Window& w, Renderer& r, AppState& s) {
	if (detectEvents(s)) draw(w,r,s);
}

int main() {
	std::ifstream config {"config.txt"};
	std::string title = "Sketch Client";
	if (config) std::getline(config, title);

	static // Emscripten destructs this early if it's not set static.
	Window window {title.c_str(), 800, 600};
	AppState state {};
	Renderer renderer {
		window.pixels, window.width(), window.height(),
		[=](Col3 c) -> uint32_t {
			return SDL_MapRGB(window.format, c.r, c.g, c.b);
		},
		[=](uint32_t pixel) -> Col3 {
			Col3 c;
			SDL_GetRGB(pixel, window.format, &c.r, &c.g, &c.b);
			return c;
		}
	};

	std::ifstream input {"example raw.sketch"};
	if (!input) {
		std::cerr << "File not found!.\n";
	}
	else if (RawFormat::verify(input)) {
		input.clear();
		input.seekg(0);
		state.example = RawFormat::parse(input);
	}
	else {
		std::cout << "parse failed :(\n";
	}

#	ifdef __EMSCRIPTEN__
		JS::listenForPenPressure();
		// JS::listenForClipboard();

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
#	else
		while (!state.quit) appLoopBody(window, state);
#	endif
}