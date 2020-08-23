CPP = g++
CPPFLAGS = -I./engine -I./parser -g -O2
LDFLAGS = -lboost_program_options

SP_SCENERY = scenery/sp-school-capacity-100.exec scenery/sp-school-capacity-33.exec scenery/sp-school-capacity-66.exec

OBJS_ENGINE := $(patsubst %.cpp,%.o,$(wildcard engine/*.cpp))
OBJS_PARSER := $(patsubst %.cpp,%.o,$(wildcard parser/*.cpp))
OBJS_SCENERY := $(patsubst %.cpp,%.o,$(wildcard scenery/*.cpp))

OBJS := $(OBJS_ENGINE) $(OBJS_PARSER)
ALL_OBJS := $(OBJS) $(OBJS_SCENERY)

BINARIES := $(patsubst %.cpp,%.exec,$(wildcard scenery/*.cpp)) $(SP_SCENERY)

HEADERS = $(wildcard engine/*.h) $(wildcard parser/*.h)

%.o: %.cpp $(HEADERS)
	$(CPP) $(CPPFLAGS) -c -o $@ $<

%.exec: %.o $(OBJS)
	$(CPP) $(CPPFLAGS) -o $@ $< $(OBJS) $(LDFLAGS)

all: $(ALL_OBJS) $(BINARIES)
	@echo "Everything compiled! yes!"

scenery/sp-school-capacity-100.exec: scenery/sp.cpp $(OBJS) $(HEADERS)
	$(CPP) $(CPPFLAGS) -DSCHOOL_STRATEGY=0 -o $@ $< $(OBJS) $(LDFLAGS)

scenery/sp-school-capacity-33.exec: scenery/sp.cpp $(OBJS) $(HEADERS)
	$(CPP) $(CPPFLAGS) -DSCHOOL_STRATEGY=1 -o $@ $< $(OBJS) $(LDFLAGS)

scenery/sp-school-capacity-66.exec: scenery/sp.cpp $(OBJS) $(HEADERS)
	$(CPP) $(CPPFLAGS) -DSCHOOL_STRATEGY=2 -o $@ $< $(OBJS) $(LDFLAGS)

#scenery/sp-school-opening-day.exec: scenery/sp.cpp $(OBJS) $(HEADERS)
#	$(CPP) $(CPPFLAGS) -DSCHOOL_STRATEGY=1 -DSP_CYCLE_TO_OPEN_SCHOOL=210.0 -DSP_RESULTS_FILE=\"210\" -o $@ $< $(OBJS) $(LDFLAGS)

test: $(OBJS) test.cpp
	$(CPP) $(CPPFLAGS) -o test $(OBJS) test.cpp
	@echo "Test compiled! yes!"

clean:
	- rm -rf $(BINARIES) $(ALL_OBJS) test
