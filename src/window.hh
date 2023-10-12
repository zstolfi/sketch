#pragma once
#include <SDL2/SDL.h>
#include <span>
#include <cctype>

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
	void setSize  (Dimension d) {
		m_W = d.W;
		m_H = d.H;
		// TODO: look up the right SDL calls and such
	}
	void updatePixels() { SDL_UpdateWindowSurface(sdlWindow); }
};