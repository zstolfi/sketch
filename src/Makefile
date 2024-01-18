CC        = emcc
CXX       = em++
CXXFLAGS  = -std=c++23 -Wall
LDFLAGS   = -sUSE_SDL=2
FUNCTIONS = _main,_jsSetPenPressure,_jsSetClipboard,_jsGetClipboard

INPUT     = ../web/input@/
TARGET    = ../web/output/sketch.html

SOURCES   = $(wildcard *.cc)
OBJECTS   = $(SOURCES:.cc=.o)
LDFLAGS  += -sEXPORTED_FUNCTIONS=$(FUNCTIONS)
LDFLAGS  += -sEXPORTED_RUNTIME_METHODS=ccall,cwrap
LDFLAGS  += --embed-file $(INPUT)

all : $(SOURCES) $(TARGET)

$(TARGET) : $(OBJECTS)
	$(CXX) $(LDFLAGS) $(OBJECTS) -o $@

.cc.o :
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean :
	rm -f *.o $(basename $(TARGET)).*