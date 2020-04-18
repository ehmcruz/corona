CPP = g++
CPPFLAGS = -I./engine -I./parser -g

OBJS_ENGINE := $(patsubst %.cpp,%.o,$(wildcard engine/*.cpp))
OBJS_PARSER := $(patsubst %.cpp,%.o,$(wildcard parser/*.cpp))
OBJS_SCENERY := $(patsubst %.cpp,%.o,$(wildcard scenery/*.cpp))

OBJS := $(OBJS_ENGINE) $(OBJS_PARSER)
ALL_OBJS := $(OBJS) $(OBJS_SCENERY)

BINARIES := $(patsubst %.cpp,%.exec,$(wildcard scenery/*.cpp))

HEADERS = $(wildcard engine/*.h) $(wildcard parser/*.h)

%.o: %.cpp $(HEADERS)
	$(CPP) $(CPPFLAGS) -c -o $@ $<

%.exec: %.o $(OBJS)
	$(CPP) $(CPPFLAGS) -o $@ $< $(OBJS)

all: $(ALL_OBJS) $(BINARIES)
	@echo "Everything compiled! yes!"

test: $(OBJS) test.cpp
	$(CPP) $(CPPFLAGS) -o test $(OBJS) test.cpp

clean:
	- rm -rf $(BINARIES) $(ALL_OBJS) test
