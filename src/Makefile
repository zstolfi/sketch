FUNCTIONS = -sEXPORTED_FUNCTIONS=_main,_jsSetPenPressure,_jsSetClipboard,_jsGetClipboard -sEXPORTED_RUNTIME_METHODS=cwrap

EXPORTED_FUNCTIONS = -static-libgcc -static-libstdc++

COMPILER_FLAGS = -std=c++23 -sUSE_SDL=2

INPUT = --embed-file ../web/input@/

OUTPUT = -o ../web/output/sketch.html

all :
	copy ".\tests\example file.hsc" "..\web\input\example file.hsc"
	copy ".\tests\example raw.sketch" "..\web\input\example raw.sketch"
	em++ main.cc $(COMPILER_FLAGS) $(FUNCTIONS) $(INPUT) $(OUTPUT)