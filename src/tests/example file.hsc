% ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ %
% Sketch Client file format (.hsc) %
% ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ %

% Every line starting with a '%' is a comment.
% Comments are single line only.

% A Sketch file is a series of comma-separated elements with
% 0 or more modifiers following each element. The final
% element is terminated with a semicolon.

% The following are valid sketch file structures.
% ;
% Elem ;
% Elem Mod ;
% Elem , Elem , Elem ;
% Elem , Elem Mod Mod , Elem ;

% Each element and modifier is preceded by a type.
% The element/modifier data is always surrounded by either
% [], or () brackets (including empty data).

% Flattened data is encoded as base-36 triples (tick marks optional)
% Negative values are allowed, and enocded like so:
% (000, hzz) -> (+0, +23327)
% (i00, zzz) -> (-23328, -1)
%       (x,y)---(x,y)---(x,y)   (x,y)--- etc...
Data [ 042'02o'05g'02q'051'04c 05h'01l'05a'01g'04m'01a'03u'01b'02x'026'02v'039'03d'03s'04c'03y'053'03p ],

% Raw ".sketch" data can be converted into flattened data:
Raw [ 0000m800m8go00go0000 ],

% Brush data is encoded by alternating stroke diamaters
% and strokes. The stroke diameters are 2 digit base-36
% integers, and the stroke points are 8 base-36 digits each.
% (The last two denote that point's pressure.)
% (00, zz) -> (0.0, 1.0)

%       dd xxxyyy pp xxxyyy pp etc...
Brush [ 0a 06m01g'k0'06m01r'l5'06l038'k8'06g04g'e0 0a 06m035'a0'06v033'a0'07d030'a0'08902s'a0 0a 08e01d'k0'08e01o'k0'08f01z'k0'08d047'k0'08b04n'k0 ],

% Transformations are allowed to be applied to elements.
% Here a square is translated and rotated by 30° clockwise.
Data [ zzkzzk'00gzzk'00g00g'zzk00g'zzkzzk ]
	Affine [
		+0.866  -0.500  391
		+0.500  +0.866  104
		     0       0    1
	],

% Chessboard:
Data [ 000000'00k000 000002'00k002 000004'00k004 000006'00k006 000008'00k008 00000a'00k00a 00000c'00k00c 00000e'00k00e 00000g'00k00g 00000i'00k00i 00000k'00k00k ]
	Array [ 2 Affine [ 1 0 20 0 1 20 0 0 1 ] ]
	Array [ 4 Affine [ 1 0 40 0 1 0  0 0 1 ] ]
	Array [ 4 Affine [ 1 0 0  0 1 40 0 0 1 ] ]
,

% Pencil is just a synonym for Data currently.
Pencil [ 0c902k0cc02i0ch02d0cg01k0c901a0ca01a0cl01d0d601c0df0170dg0170dm01s0ds0330dm0490d804q0cn04o0cp0410eg032 ],

% String literals are in nestable parens, postscript-style.
Marker ("The quick brown fox jumps over the lazy dog!!") Uppercase [],
Marker (Shouted the text-modified marker.),
Marker (This marker is the (final) element of the sketch.);

%	Eraser
%		TODO ...
%	Pattern
%		TODO ...