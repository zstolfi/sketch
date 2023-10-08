/*
	TODO: Makefile
	em++ sdl_canvas_test.cc -sUSE_SDL=2 -sEXPORTED_FUNCTIONS=_main,_setWinPressure -sEXPORTED_RUNTIME_METHODS=cwrap -o test.html
*/

#include <SDL2/SDL.h>
#include <emscripten.h>
#include <emscripten/html5.h>

#include <iostream>
#include <cmath>

unsigned W = 800, H = 600;
SDL_Window* window;
SDL_Surface* surface;
bool quit = false;

struct AppState {
	bool penPressed = false;
	int penX = 0;
	int penY = 0;
	float penPressure = 1.0;
	unsigned penSize = 10;
};

// SDL2 can't detect pen pressure it seems, we
// use some Javascript to do that work for us.
void startPressureListener() {
	EM_ASM(
		const setWinPressure = Module.cwrap("setWinPressure", "", ["number"]);
		Module.canvas.addEventListener("pointerdown", event => setWinPressure(event.pressure));
		Module.canvas.addEventListener("pointermove", event => setWinPressure(event.pressure));
		Module.canvas.addEventListener("pointerup", event => setWinPressure(event.pressure));
	);
}

float winPressure = 1.0;
extern "C" {
	void setWinPressure(float pressure) {
		winPressure = pressure;
	}
}


void draw(const AppState& s) {
	unsigned* pixels = (unsigned*)surface->pixels;

	for (unsigned y=0, i=0; y < H; y++     )
	for (unsigned x=0     ; x < W; x++, i++) {
		unsigned char r=200, g=200, b=200;

		int dx = s.penX - x;
		int dy = s.penY - y;
		int sz = s.penSize * (s.penPressed ? s.penPressure : 1.0);
		if (dx*dx + dy*dy <= sz*sz) {
			if (!s.penPressed) r=255, g=  0, b=128;
			else               r=  0, g=  0, b=  0;
		}
		pixels[i] = SDL_MapRGB(surface->format, r, g, b);
	}

	SDL_UpdateWindowSurface(window);
}

bool detectEvents(AppState& s) {
	bool input = false;
	for (SDL_Event ev; SDL_PollEvent(&ev); input=true)
	switch (ev.type) {
		case SDL_QUIT:
			quit = true;
			break;
		case SDL_MOUSEMOTION:
			s.penX = ev.motion.x;
			s.penY = ev.motion.y;
			s.penPressure = winPressure;
			break;
		case SDL_MOUSEBUTTONDOWN:
			s.penPressed = true;
			s.penPressure = winPressure;
			break;
		case SDL_MOUSEBUTTONUP:
			s.penPressed = false;
			s.penPressure = 0.0;
			break;
		case SDL_KEYDOWN:
			switch (ev.key.keysym.sym) {
				case SDLK_ESCAPE:
					quit = true;
					break;
				case SDLK_LEFTBRACKET:
					s.penSize = std::max(0u, s.penSize-1);
					break;
				case SDLK_RIGHTBRACKET:
					s.penSize = std::min(20u, s.penSize+1);
					break;
			} break;
	}
	return input;
}

void appLoopBody(AppState& s) {
	if (detectEvents(s)) draw(s);
}

int main(int argc, char* argv[]) {
	SDL_Init(SDL_INIT_VIDEO);

	window = SDL_CreateWindow("Sketch Client",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		W, H, SDL_WINDOW_SHOWN);
	surface = SDL_GetWindowSurface(window);

	AppState state {};

#ifdef __EMSCRIPTEN__
	startPressureListener();

	emscripten_set_main_loop_arg(
		[](void* s) {
			appLoopBody(*(AppState*)s);
		},
		&state,
		0, true
	);
#else
	while (!quit) appLoopBody(state);
#endif

	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}