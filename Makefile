SRCS = $(wildcard *.cpp)
HEADERS = $(wildcard *.h)

all: $(SRCS) $(HEADERS)
	g++ -o corona $(SRCS)

clean:
	rm corona
