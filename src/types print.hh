#pragma once
#include <iostream>
#include "types.hh"

std::ostream& operator<<(std::ostream& os, const Point& p) {
	os << "(" << p.x << ", "  << p.y << ", "  << p.pressure << ")";
	return os;
}

std::ostream& operator<<(std::ostream& os, const Stroke& s) {
	os << "width: " << s.diameter << "\n";
	os << "points:";
	for (Point p : s.points)
		os << "\t" << p;
	return os;
}

std::ostream& operator<<(std::ostream& os, const Marker& m) {
	os << "message: " << m.text;
	return os;
}

std::ostream& operator<<(std::ostream& os, const Sketch& s) {
	auto groupIt = s.elements.begin();

	for (auto it=s.atoms.begin(); it!=s.atoms.end(); ++it) {
		if (std::holds_alternative<Stroke>(*it))
			os << std::get<Stroke>(*it);
		else if (std::holds_alternative<Marker>(*it))
			os << std::get<Marker>(*it);
		/* ... */

		if (std::distance(it, s.atoms.end())) os << "\n";
	}
	return os;
}