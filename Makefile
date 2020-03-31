CPP = g++
CPPFLAGS = -I./engine

OBJS := $(patsubst %.cpp,%.o,$(wildcard engine/*.cpp)) $(patsubst %.cpp,%.o,$(wildcard scenery/*.cpp)) 

BINARIES := $(patsubst %.cpp,%.exec,$(wildcard scenery/*.cpp))

HEADERS = $(wildcard engine/*.h)

%.o: %.cpp $(HEADERS)
	$(CPP) $(CPPFLAGS) -c -o $@ $<

%.exec: $(OBJS)
	$(CPP) $(CPPFLAGS) -o $@ $(OBJS)

all: $(OBJS) $(BINARIES)
	@echo "Everything compiled! yes!"

clean:
	- rm -rf $(BINARIES) $(OBJS)
