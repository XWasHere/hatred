CXX=      g++
CXX_ARGS= -ggdb -O3 -fpermissive -DDEBUG --std=c++23

BACKDOOR_OBJS = \
	build/backdoor/main.o \
	build/backdoor/net.o \
	build/backdoor/upnp.o \
	build/backdoor/ssdp.o \
	build/backdoor/http.o \
	build/backdoor/xml.o

.PHONY: all backdoor clean

all: backdoor

clean:
	rm -f build/backdoor/*
	rm -f out/*
	
backdoor: out/hatred

out/hatred: $(BACKDOOR_OBJS)
	$(CXX) $(CXX_ARGS) $(BACKDOOR_OBJS) -o out/hatred

build/backdoor/main.o: src/backdoor/main.cc src/backdoor/net.h src/backdoor/util.h src/backdoor/upnp.h src/backdoor/http.h src/backdoor/ssdp.h src/backdoor/xml.h
	mkdir -p build/backdoor/
	$(CXX) $(CXX_ARGS) -c src/backdoor/main.cc -o build/backdoor/main.o

build/backdoor/net.o: src/backdoor/net.cc src/backdoor/net.h src/backdoor/util.h
	mkdir -p build/backdoor/
	$(CXX) $(CXX_ARGS) -c src/backdoor/net.cc -o build/backdoor/net.o

build/backdoor/upnp.o: src/backdoor/upnp.cc src/backdoor/net.h src/backdoor/util.h src/backdoor/upnp.h
	mkdir -p build/backdoor/
	$(CXX) $(CXX_ARGS) -c src/backdoor/upnp.cc -o build/backdoor/upnp.o

build/backdoor/ssdp.o: src/backdoor/ssdp.cc src/backdoor/net.h src/backdoor/ssdp.h
	mkdir -p build/backdoor/
	$(CXX) $(CXX_ARGS) -c src/backdoor/ssdp.cc -o build/backdoor/ssdp.o

build/backdoor/http.o: src/backdoor/http.cc src/backdoor/net.h src/backdoor/http.h
	mkdir -p build/backdoor/
	$(CXX) $(CXX_ARGS) -c src/backdoor/http.cc -o build/backdoor/http.o

build/backdoor/xml.o: src/backdoor/xml.cc src/backdoor/xml.h
	mkdir -p build/backdoor/
	$(CXX) $(CXX_ARGS) -c src/backdoor/xml.cc -o build/backdoor/xml.o