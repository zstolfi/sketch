/*
	TODO: Makefile
	em++ sdl_canvas_test.cc -std=c++20 -sUSE_SDL=2 -sEXPORTED_FUNCTIONS=_main,_setJsPressure -sEXPORTED_RUNTIME_METHODS=cwrap -o test.html
*/

#include <SDL2/SDL.h>
#include <emscripten.h>
#include <emscripten/html5.h>

#include <iostream>
#include <span>
#include <cmath>
#include <cstdint>

class Window {
	unsigned m_W, m_H;
	SDL_Window* sdlWindow;
	SDL_Surface* sdlSurface;

public:
	Window(const char* title, unsigned W, unsigned H)
	: m_W{W}, m_H{H} {
		// Initialize SDL stuff on window construction
		SDL_Init(SDL_INIT_VIDEO);
		sdlWindow = SDL_CreateWindow(title,
			SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
			W, H, SDL_WINDOW_SHOWN);
		sdlSurface = SDL_GetWindowSurface(sdlWindow);

		// Convert void* array to a span<uint32>
		// because type-less pointers are yucky.
		uint32_t* start = (uint32_t*)sdlSurface->pixels;
		auto size = sdlSurface->h * sdlSurface->pitch;
		pixels = {start, start+size};

		format = sdlSurface->format;
	}

	~Window() {
		SDL_DestroyWindow(sdlWindow);
		SDL_Quit();
	}

	// SDL weirdly uses pointers to pixel formats
	// instead of just having the data on its own
	SDL_PixelFormat* format;
	std::span<uint32_t> pixels;

	// Getters
	struct Dimension { unsigned W, H; };
	unsigned  width () const { return m_W; }
	unsigned  height() const { return m_H; }
	Dimension size  () const { return {m_W, m_H}; }

	// Setters
	void setSize  (Dimension d) { m_W = d.W, m_H = d.H; }
	void updatePixels() { SDL_UpdateWindowSurface(sdlWindow); }

};

struct AppState {
	bool quit = false;

	bool penPressed = false;
	int penX = 0;
	int penY = 0;
	float penPressure = 1.0;
	unsigned penSize = 10;

	/* Most app data will go here. */
};



// SDL2 can't detect any pen pressure, so
// we use some Javascript to help us out.
static float jsPressure = 1.0;
extern "C" {
	void setJsPressure(float pressure) {
		jsPressure = pressure;
	}
}

// https://discourse.libsdl.org/t/get-tablet-stylus-pressure/35319/2
void startPressureListener() {
	EM_ASM(
		const setJsPressure = Module.cwrap("setJsPressure", "", ["number"]);
		Module.canvas.addEventListener("pointerdown", event => setJsPressure(event.pressure));
		Module.canvas.addEventListener("pointermove", event => setJsPressure(event.pressure));
		Module.canvas.addEventListener("pointerup", event => setJsPressure(event.pressure));
	);
}



void draw(Window& w, const AppState& s) {
	for (unsigned y=0, i=0; y < w.height(); y++     )
	for (unsigned x=0     ; x < w.width (); x++, i++) {
		unsigned char r=200, g=200, b=200;

		int dx = s.penX - x;
		int dy = s.penY - y;
		int sz = s.penSize * (s.penPressed ? s.penPressure : 1.0);
		if (dx*dx + dy*dy <= sz*sz) {
			if (!s.penPressed) r=255, g=  0, b=128;
			else               r=  0, g=  0, b=  0;
		}
		w.pixels[i] = SDL_MapRGB(w.format, r, g, b);
	}

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
			s.penX = ev.motion.x;
			s.penY = ev.motion.y;
			s.penPressure = jsPressure;
			break;
		case SDL_MOUSEBUTTONDOWN:
			s.penPressed = true;
			s.penPressure = jsPressure;
			break;
		case SDL_MOUSEBUTTONUP:
			s.penPressed = false;
			s.penPressure = 0.0;
			break;
		case SDL_KEYDOWN:
			switch (ev.key.keysym.sym) {
				case SDLK_ESCAPE:
					s.quit = true;
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



void appLoopBody(Window& w, AppState& s) {
	if (detectEvents(s)) draw(w,s);
}

int main() {
	Window window {"Sketch Client", 800, 600};
	AppState state {};

#ifdef __EMSCRIPTEN__
	startPressureListener();

	// Unsightly lambda expression syntax... Basically
	// we want to pass in 'window' and 'state' as args
	// into the appLoopBody function with emscripten's
	// set_main function, but we're only allowed to do
	// so using a void*. I used a tuple<win*,state*>*.
	static std::tuple userData { &window, &state };
	emscripten_set_main_loop_arg(
		[](void* data) {
			auto [w,s] = *(decltype(userData)*)data;
			appLoopBody(*w,*s);
		},
		&userData,
		0, true
	);
#else
	while (!state.quit) appLoopBody(window, state);
#endif
}