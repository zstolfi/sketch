#pragma once
#include "math.hh"

struct Col3 { uint8_t r, g, b; };
struct Vec2 { Real x, y; };

Real dot (Vec2 a, Vec2 b);
Real dot2(Vec2 a);
Real len (Vec2 a);
Vec2 norm(Vec2 a);

Real SDFline(Vec2 p, Vec2 a, Vec2 b);