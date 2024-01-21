#include <emscripten.h>
#include <SDL2/SDL.h>
#include <iostream>
#include <fstream>
#include <optional>
#include "window.hh"
#include "external.hh"
#include "renderer.hh"
#include "parser.hh"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

struct AppState {
	bool quit = false;
	
	RawSketch example;
	std::optional<Point> cursor; // nullopt if off screen
	bool pressed = false;
};

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

void draw(Window& w, Renderer& r, const AppState& s) {
	if (!s.cursor) {
		std::cout << "Cursor is off screen!\n";
	}
	else {
		std::cout << (s.pressed ? "DOWN" : "UP") << "\t";
		std::cout << "x: " << s.cursor->x << "\t"
		          << "y: " << s.cursor->y << "\n";
	}
	r.clear();
	r.displayRaw(s.example.strokes);
	w.updatePixels();
}

bool detectEvents(AppState& s) {
	bool input = false;
	for (SDL_Event ev; SDL_PollEvent(&ev); input=true)
	switch (ev.type) {
		if (SDL_GetMouseFocus() == nullptr) s.cursor = std::nullopt;
		case SDL_QUIT:
			s.quit = true;
			break;
		case SDL_MOUSEMOTION:
			s.cursor = {
				ev.motion.x,
				ev.motion.y,
				JS::penPressure
			};
			break;
		case SDL_MOUSEBUTTONDOWN:
			s.pressed = true;
			s.cursor->pressure = JS::penPressure;
			break;
		case SDL_MOUSEBUTTONUP:
			s.pressed = false;
			s.cursor->pressure = 0.0;
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
	}
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

	static Window window {title.c_str(), 800, 600};
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
		std::string inString {it, end};
		auto tokens = SketchFormat::tokenize(inString);
		std::cout << "\n#### TOKENS ####\n";
		for (auto t : tokens)
			std::cout << "\t\"" << t << "\"\n";

		std::cout << "\n#### ELEMENTS ####\n";
		if (auto sketch = SketchFormat::parse(tokens)) {
			std::cout << *sketch << "\n";
			state.example = sketch->flatten();
			std::cout << "And as a char stream:\n";
			// accidentally re-invented the wheel here
			sketch->flatten().sendTo(std::cout);
		}
		std::cout << "\n#### RAW PARSE TESTS ####\n";
		auto parseAndPrint = [&](std::string_view str) {
			std::cout << "input: '" << str << "'\n";
			if (auto s = RawFormat::parse(str)) {
				std::cout << "\t" << *s << "\n";
			} else {
				std::cout << "\t! Invalid .sketch !\n";
			}
		};
		parseAndPrint("");
		parseAndPrint("abcd");
		parseAndPrint("123456");
		parseAndPrint("  123456  ");
		parseAndPrint(  "1234  ");
		parseAndPrint("  1234"  );
		parseAndPrint("  1234  ");
		parseAndPrint("00000202 0808!a0a");
		parseAndPrint( // 7352439
			"2j5i2l5m2w653c743v8c48974b9h 1f541f521q4o2g433h3i4g3b4z3g5d3"
			"r5q455y4k60555x5o5k64526h4m6o466w406z4070 6m5m6t5t796m7j7b7n"
			"7n7o7p 6o5i6s5f7d5a8b568w558x55 7g6e7v698n648u63 7j7j7l7j8f7"
			"d9a7d9f7e 9q7n9r7ja177ag6oay5zbc5hbi5cbl5abp59bx5acc5kcx68di"
			"71dt7hdx7n b06tb16tbq6qcf6och6o ek7rem7rem7kej6zeh64eh5heh5e"
			"el5mf16bfi74g17tg77yga7pgc76gf6hgh5qgi5cgi5b hd68he68hf6hhe6"
			"yhe7ghm83hu8ei48lil8oj38jjd86jf7qjb79j56vj36mj36j li7glh7dl8"
			"74kp6uk76tjy74jt7mjs7vjx83k288kc8bks8hl38nld8vle91lc98l59iku"
			"9pkg9pk29pju9njr9ljq9k 5w9e5z9e6z958v8zbj8ydu8xfs90h795i69bi"
			"w9jj29kiy9kib9jgk9ldc9xa2ab8eaj86an86ao8hat98auawawd8b3eubbf"
			"3bee6bic3brbjbxclc6ducgdtcnd5cwd2cxd1d0cwdgcwdicudkcsdo"
		);
		std::cout << "\n#### END ####\n";
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
		while (!state.quit) appLoopBody(window, renderer, state);
#	endif
}