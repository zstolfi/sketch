#pragma once
#include <SDL2/SDL.h>
#include <string_view>
#include <span>
#include <cctype>

class Window {
	unsigned m_W, m_H;
	SDL_Window* sdlWindow;
	SDL_Surface* sdlSurface;

public:
	Window(std::string_view title, unsigned W, unsigned H);
	~Window();

	// SDL weirdly uses pointers to pixel formats
	// instead of just having the data on its own
	SDL_PixelFormat* format;
	std::span<uint32_t> pixels;

	// Getters
	struct Dimension { unsigned W, H; };
	unsigned  width () const;
	unsigned  height() const;
	Dimension size  () const;

	// Setters
	void setSize  (Dimension);
	void updatePixels();
};