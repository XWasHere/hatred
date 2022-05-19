CXX=      g++
CXX_ARGS= -ggdb -O3 -fpermissive -DDEBUG

BACKDOOR_OBJS = \
	build/backdoor/main.o \
	build/backdoor/net.o \
	build/backdoor/upnp.o 

.PHONY: all backdoor clean

all: backdoor

clean:
	rm -f build/backdoor/*
	rm -f out/*
	
backdoor: out/hatred

out/hatred: $(BACKDOOR_OBJS)
	$(CXX) $(CXX_ARGS) $(BACKDOOR_OBJS) -o out/hatred

build/backdoor/main.o: src/backdoor/main.cc src/backdoor/net.h src/backdoor/util.h src/backdoor/upnp.h
	mkdir -p build/backdoor/
	$(CXX) $(CXX_ARGS) -c src/backdoor/main.cc -o build/backdoor/main.o

build/backdoor/net.o: src/backdoor/net.cc src/backdoor/net.h src/backdoor/util.h
	mkdir -p build/backdoor/
	$(CXX) $(CXX_ARGS) -c src/backdoor/net.cc -o build/backdoor/net.o

build/backdoor/upnp.o: src/backdoor/upnp.cc src/backdoor/net.h src/backdoor/util.h src/backdoor/upnp.h
	mkdir -p build/backdoor/
	$(CXX) $(CXX_ARGS) -c src/backdoor/upnp.cc -o build/backdoor/upnp.o