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

	struct Signal {
		bool mouseDown:1 {}, mouseMove:1 {}, mouseUp:1 {};
		bool undo:1 {}, redo:1 {};

		void clear() { *this = Signal {}; }
	} signal;
	
	class History {
		std::vector<const Sketch> states {Sketch {}};
		std::size_t head = 0;
	public:
		const Sketch& view() { return states[head]; }
		void look_back   () { head -= (head > 0); }
		void look_forward() { head += (head < states.size()-1); }
		void push(const Sketch& s) {
			states.resize(++head);
			states.push_back(s);
		}
	} history;

	Atom::Stroke currentStroke {};

	Atom::Stroke::Point cursor {0, 0, 0.0};
	bool pressed = false;
	bool onScreen = false;
	unsigned brushSize = 3;
};

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

void draw(Window& w, Renderer& r, AppState& s) {
	if (s.signal.undo) s.history.look_back();
	if (s.signal.redo) s.history.look_forward();

	if (s.signal.mouseDown) {
		s.currentStroke.points.clear();
		s.currentStroke.diameter = s.brushSize;
		s.currentStroke.points.push_back(s.cursor);
	}

	if (s.signal.mouseMove && s.pressed) {
		s.currentStroke.points.push_back(s.cursor);
	}

	if (s.signal.mouseUp) {
		Sketch nextState = s.history.view();
		auto& elements = nextState.elements;

		if (elements.size() == 0
		||  !std::holds_alternative<Brush>(elements.back())) {
			elements.push_back(Brush {});
		}

		Brush& latestBrush = std::get<Brush>(elements.back());
		latestBrush.atoms.push_back(
			std::move(s.currentStroke)
		);

		s.currentStroke.points.clear();
		s.history.push(nextState);
	}

	r.clear();
	r.displayRaw(s.history.view().render().strokes);
	w.updatePixels();
}

bool detectEvents(AppState& s) {
	s.onScreen = SDL_GetMouseFocus() == nullptr;
	s.signal.clear();

	bool input = false;
	for (SDL_Event ev; SDL_PollEvent(&ev); input=true) {
	switch (ev.type) {
	case SDL_QUIT:
		s.quit = true;
		break;
	case SDL_MOUSEMOTION:
		s.signal.mouseMove = true;
		s.cursor = {
			ev.motion.x,
			ev.motion.y,
			JS::penPressure,
		};
		break;
	case SDL_MOUSEBUTTONDOWN:
		s.signal.mouseDown = true;
		s.pressed = true;
		s.cursor.pressure = JS::penPressure;
		break;
	case SDL_MOUSEBUTTONUP:
		s.signal.mouseUp = true;
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
		case SDLK_z:
			s.signal.undo = true;
			break;
		case SDLK_y:
			s.signal.redo = true;
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
			state.history.push(*sketch);
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