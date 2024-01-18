#include "window.hh"
#include <iostream>

Window::Window(const char* title, unsigned W, unsigned H)
: m_W{W}, m_H{H} {
	// Initialize SDL stuff on window construction
	std::cout << "Initializing SDL video...\n";
	SDL_Init(SDL_INIT_VIDEO);
	std::cout << "Creating SDL window...\n";
	sdlWindow = SDL_CreateWindow(title,
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		W, H, SDL_WINDOW_SHOWN);
	std::cout << "Getting SDL window surface...\n";
	sdlSurface = SDL_GetWindowSurface(sdlWindow);

	// Convert void* array to a span<uint32>
	// because type-less pointers are yucky.
	uint32_t* start = (uint32_t*)sdlSurface->pixels;
	auto size = sdlSurface->h * sdlSurface->pitch;
	pixels = {start, start+size};

	format = sdlSurface->format;
}

Window::~Window() {
	std::cout << "Destroying SDL window...\n";
	SDL_DestroyWindow(sdlWindow);
	std::cout << "Quitting SDL...\n";
	SDL_Quit();
}

// Getters
unsigned          Window::width () const { return m_W; }
unsigned          Window::height() const { return m_H; }
Window::Dimension Window::size  () const { return {m_W, m_H}; }

// Setters
void Window::setSize  (Dimension d) {
	m_W = d.W;
	m_H = d.H;
	// TODO: look up the right SDL calls and such
}
void Window::updatePixels() { SDL_UpdateWindowSurface(sdlWindow); }
