% ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ %
% Sketch Client file format (.hsc) %
% ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ %

% Every line starting with a '%' is a comment.
% Comments are single line only.

% Raw data is encoded as base-36 triples (tick marks optional)
% Negative values are allowed, but anything outside the bounds
% of the 800 x 600 px canvas won't be rendered.
% (000, hzz) -> (+0, +23327)
% (i00, zzz) -> (-23328, -1)
%         (x,y)---(x,y)---(x,y)   (x,y)--- etc...
Data : [ 042'02o'05g'02q'051'04c 05h'01l'05a'01g'04m'01a'03u'01b'02x'026'02v'039'03d'03s'04c'03y'053'03p ],
% The comma at the end separates two elements. If it were a 
% semicolon instead it would mark the end of the file.


% Brush data is encoded by alternating stroke diamaters
% and strokes. The stroke diameters are unsigned base 10
% integers, and the stroke points are 8 base-36 digits each.
% (The extra two denote that point's pressure.)
% (00, zz) -> (0.0, 1.0)

%         dd xxxyyy pp xxxyyy pp etc...
Brush : [ 10 06m01g'k0'06m01r'l5'06l038'k8'06g04g'e0 10 06m035'a0'06v033'a0'07d030'a0'08902s'a0 10 08e01d'k0'08e01o'k0'08f01z'k0'08d047'k0'08b04n'k0 ]

% Transformations are allowed to be applied to elements.
% Here a square is translated and rotated by 30° clockwise.
Data : [ zzkzzk'016zzk'016016'zzk016'zzkzzk ]
	Affine : [
		+0.866, -0.500, 391,
		+0.500, +0.000, 104,
		    0,     0,   1
	],

% Pencil is just a synonym for Data currently.
Pencil : [ 0c902k0cc02i0ch02d0cg01k0c901a0ca01a0cl01d0d601c0df0170dg0170dm01s0ds0330dm0490d804q0cn04o0cp0410eg032 ],

% String literals are in nest-able parens, postscript-style.
Marker : (This is a marker at the end of the file.);
%	Erase : 
%		...