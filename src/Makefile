CC        = emcc
CXX       = em++
CXXFLAGS  = -std=c++23 -Wall
LDFLAGS   = -sUSE_SDL=2
FUNCTIONS = _main,_jsSetPenPressure,_jsSetClipboard,_jsGetClipboard

INPUT     = ../web/input@/
OUTPUT    = ../web/output/
TARGET    = $(OUTPUT)sketch.js
INDEXHTML = $(OUTPUT)index.html

SOURCES   = $(wildcard *.cc)
OBJECTS   = $(SOURCES:.cc=.o)
LDFLAGS  += -sEXPORTED_FUNCTIONS=$(FUNCTIONS)
LDFLAGS  += -sEXPORTED_RUNTIME_METHODS=ccall,cwrap
LDFLAGS  += --embed-file $(INPUT)

all : $(SOURCES) $(TARGET) $(INDEXHTML)

$(TARGET) : $(OBJECTS)
	$(CXX) $(LDFLAGS) $(OBJECTS) -o $@

.cc.o :
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(INDEXHTML) : ./webpage/index.html
	cp ./webpage/index.html $(INDEXHTML)

clean :
	rm -f *.o $(basename $(TARGET)).* $(INDEXHTML)