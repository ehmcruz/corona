CPP = g++
CPPFLAGS = -I./engine -I./parser -g -O2
LDFLAGS = -lboost_program_options

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
	$(CPP) $(CPPFLAGS) -o $@ $< $(OBJS) $(LDFLAGS)

all: $(ALL_OBJS) $(BINARIES)
	@echo "Everything compiled! yes!"

test: $(OBJS) test.cpp
	$(CPP) $(CPPFLAGS) -o test $(OBJS) test.cpp
	@echo "Test compiled! yes!"

clean:
	- rm -rf $(BINARIES) $(ALL_OBJS) test
