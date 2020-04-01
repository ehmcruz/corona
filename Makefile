CPP = g++
CPPFLAGS = -I./engine

OBJS_ENGINE := $(patsubst %.cpp,%.o,$(wildcard engine/*.cpp))
OBJS_SCENERY := $(patsubst %.cpp,%.o,$(wildcard scenery/*.cpp))
OBJS := $(patsubst %.cpp,%.o,$(wildcard engine/*.cpp)) $(patsubst %.cpp,%.o,$(wildcard scenery/*.cpp))
BINARIES := $(patsubst %.cpp,%.exec,$(wildcard scenery/*.cpp))

HEADERS = $(wildcard engine/*.h)

%.o: %.cpp $(HEADERS)
	$(CPP) $(CPPFLAGS) -c -o $@ $<

%.exec: %.o $(OBJS)
	$(CPP) $(CPPFLAGS) -o $@ $< $(OBJS_ENGINE)

all: $(OBJS_ENGINE) $(OBJS_SCENERY) $(BINARIES)
	@echo "Everything compiled! yes!"

clean:
	- rm -rf $(BINARIES) $(OBJS_ENGINE) $(OBJS_SCENERY)
