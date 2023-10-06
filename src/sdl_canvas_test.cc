#include <SDL2/SDL.h>
#include <emscripten.h>
#include <cmath>

unsigned W = 800, H = 600;
SDL_Window* window;
SDL_Surface* surface;
int cursorX = 0, cursorY = 0;
bool quit = false;

void draw() {
	unsigned* pixels = (unsigned*)surface->pixels;
	for (std::size_t i=0; i < W*H; i++) {
		int x = i % W;
		int y = i / W;
		if (std::hypot(cursorX - x, cursorY - y) <= 10)
			pixels[i] = SDL_MapRGB(surface->format, 255, 0, 128);
		else
			pixels[i] = SDL_MapRGB(surface->format, 200, 200, 200);

	}

	SDL_UpdateWindowSurface(window);
}

bool detectEvents() {
	bool input = true;
	for (SDL_Event ev; SDL_PollEvent(&ev); )
	switch (ev.type) {
		case SDL_QUIT:
			quit = true; input = false;
			break;
		case SDL_MOUSEMOTION:
			cursorX = ev.motion.x;
			cursorY = ev.motion.y;
		case SDL_KEYDOWN:
			switch (ev.key.keysym.sym) {
				case SDLK_ESCAPE:
					quit = true; input = false;
					break;
			} break;
	}
	return input;
}

void gameLoop() {
	if (detectEvents()) draw();
}

int main(int argc, char* argv[]) {
	SDL_Init(SDL_INIT_VIDEO);

	window = SDL_CreateWindow("Sketch Client",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		W, H,
		0);
	surface = SDL_GetWindowSurface(window);

	draw();

#ifdef __EMSCRIPTEN__
	emscripten_set_main_loop(gameLoop, 0, true);
#else
	while (!quit) gameLoop();
#endif

	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}