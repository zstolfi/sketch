#include <emscripten.h>
#include <SDL2/SDL.h>

#include "window.hh"
#include "external.hh"
#include "renderer.hh"
#include "parsers.hh"
#include <iostream>
#include <fstream>

struct AppState {
	bool quit = false;

	// Signals (will refactor soon)
	bool mouseDown {}, mouseMove {}, mouseUp {};
	
	FlatSketch example {};
	Sketch sketch {};
	Atom::Stroke currentStroke {};

	Atom::Stroke::Point cursor {0, 0, 0.0};
	bool pressed = false;
	bool onScreen = false;
	unsigned brushSize = 3;

};

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

void draw(Window& w, Renderer& r, AppState& s) {
	// if (!s.onScreen) {
	// 	std::cout << "Cursor is off screen!\n";
	// }
	// else {
	// 	std::cout << (s.pressed ? "DOWN" : "UP") << "\t";
	// 	std::cout << "x: " << s.cursor.x << "\t"
	// 	          << "y: " << s.cursor.y << "\t"
	// 	          << "p: " << s.cursor.pressure << "\n";
	// }

	if (s.mouseDown) {
		s.currentStroke.points.clear();
		s.currentStroke.diameter = s.brushSize;
		s.currentStroke.points.push_back(s.cursor);
	}

	if (s.mouseMove && s.pressed) {
		s.currentStroke.points.push_back(s.cursor);
	}

	auto& elements = s.sketch.elements;
	if (s.mouseUp) {
		Brush& latestBrush = 
			(elements.size() == 0
			|| !std::holds_alternative<Brush>(elements.back()))
		? (elements.push_back(Brush {}), std::get<Brush>(elements.back()))
		: std::get<Brush>(elements.back());

		latestBrush.atoms.push_back(
			Atom::Stroke {s.brushSize, std::move(s.currentStroke.points)}
		);
		s.currentStroke.points.clear();

		r.clear();
		r.displayRaw(s.sketch.render().strokes);
		w.updatePixels();
	}
}

bool detectEvents(AppState& s) {
	s.mouseDown = s.mouseMove = s.mouseUp = false;
	s.onScreen = SDL_GetMouseFocus() == nullptr;

	bool input = false;
	for (SDL_Event ev; SDL_PollEvent(&ev); input=true) {
	switch (ev.type) {
	case SDL_QUIT:
		s.quit = true;
		break;
	case SDL_MOUSEMOTION:
		s.mouseMove = true;
		s.cursor = {
			ev.motion.x,
			ev.motion.y,
			JS::penPressure,
		};
		break;
	case SDL_MOUSEBUTTONDOWN:
		s.mouseDown = true;
		s.pressed = true;
		s.cursor.pressure = JS::penPressure;
		break;
	case SDL_MOUSEBUTTONUP:
		s.mouseUp = true;
		s.pressed = false;
		s.cursor.pressure = 0.0;
		break;
	case SDL_KEYDOWN:
		switch (ev.key.keysym.sym) {
		case SDLK_ESCAPE:
			s.quit = true;
			break;
		case SDLK_c: // TODO: figure out modifier keys
			JS::copy();
			break;
		case SDLK_v:
			JS::paste();
			break;
		} break;
	} }
	return input;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

void appLoopBody(Window& w, Renderer& r, AppState& s) {
	if (detectEvents(s)) draw(w,r,s);
}

int main() {
	std::ifstream config {"config.txt"};
	std::string title = "[Offline] Sketch Client";
	if (config) std::getline(config, title);

	static Window window {title, 800, 600};
	static AppState state {};
	static Renderer renderer {
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

	// Gives a whole new meaning to 'if'stream, huh? :^)
	if (std::ifstream input {"example file.hsc"}; !input) {
		std::cerr << "File not found!.\n";
	}
	else {
		std::istreambuf_iterator<char> it {input}, end {};
		std::string inputString {it, end};
		std::cout << "\n#### TOKENS ####\n";
		SketchFormat::printTokens(inputString);
		std::cout << "\n#### ELEMENTS ####\n";
		if (auto sketch = SketchFormat::parse(inputString)) {
			std::cout << *sketch << "\n";
			state.example = sketch->render();
		}
		else {
			std::cout << "Parse error!\n";
			std::cout << "Error code: " << int(sketch.error()) << "\n";
			std::cout << "At Position: " << sketch.error().pos << "\n";
		}
		std::cout << "\n#### END ####\n";
	}

#	ifdef __EMSCRIPTEN__
		JS::listenForPenPressure();
		// JS::listenForClipboard();

		auto userData = std::tie(window, renderer, state);
		emscripten_set_main_loop_arg(
			[](void* data) {
				std::apply(appLoopBody, *(decltype(userData)*)data);
			},
			&userData,
			0, true
		);
#	else
		while (!state.quit) appLoopBody(window, renderer, state);
#	endif
}