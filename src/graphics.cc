#include "graphics.hh"

Real dot (Vec2 a, Vec2 b) { return a.x*b.x + a.y*b.y; }
Real dot2(Vec2 a) /*   */ { return dot(a,a); }
Real len (Vec2 a) /*   */ { return sqrt(dot2(a)); }
Vec2 norm(Vec2 a) /*   */ { return {a.x/len(a), a.y/len(a)}; }

// https://iquilezles.org/articles/distfunctions2d

Real SDFline(Vec2 p, Vec2 a, Vec2 b) {
	Vec2 c {p.x-a.x, p.y-a.y};
	Vec2 d {b.x-a.x, b.y-a.y};

	if (dot2(d) == 0) return len(c); // Point case

	Real h = clamp(dot(c,d)/dot2(d), 0, 1);
	return len(Vec2 {c.x - d.x*h, c.y - d.y*h});
}