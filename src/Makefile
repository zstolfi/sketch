CC        = emcc
CXX       = em++
CXXFLAGS  = -sUSE_SDL=2 -std=c++23 -Wall -ferror-limit=30
LDFLAGS   = -sUSE_SDL=2
FUNCTIONS = _main,_jsSetPenPressure,_jsSetClipboard,_jsGetClipboard

INPUT     = ../web/input@/
OUTPUT    = ../web/output/
TARGET    = $(OUTPUT)sketch.js

SOURCES   = $(wildcard *.cc)
OBJECTS   = $(SOURCES:.cc=.o)
LDFLAGS  += -sEXPORTED_FUNCTIONS=$(FUNCTIONS)
LDFLAGS  += -sEXPORTED_RUNTIME_METHODS=ccall,cwrap
LDFLAGS  += --embed-file $(INPUT)

all : $(SOURCES) $(TARGET) webpage

$(TARGET) : $(OBJECTS)
	$(CXX) $(LDFLAGS) $(OBJECTS) -o $@

%.o : %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

# I have yet to figure out how to do this correctly
webpage :
	cp ./webpage/* $(OUTPUT)

.PHONY : webpage

clean :
	rm -f *.o $(OUTPUT)*