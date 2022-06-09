CXX=      g++
CXX_ARGS= -ggdb -O3 -fpermissive -DDEBUG --std=c++23

BACKDOOR_OBJS = \
	build/backdoor/main.o \
	build/backdoor/net.o \
	build/backdoor/upnp.o \
	build/backdoor/ssdp.o \
	build/backdoor/http.o \
	build/backdoor/xml.o \
	build/proto/proto.o

CLIENT_OBJS = \
	build/proto/proto.o \
	build/client/main.o

.PHONY: all backdoor clean

all: backdoor client

clean:
	rm -f build/backdoor/*
	rm -f build/proto/*
	rm -f build/client/*
	rm -f out/*
	
backdoor: out/hatred

client: out/hatredctl

out/hatred: $(BACKDOOR_OBJS)
	$(CXX) $(CXX_ARGS) $(BACKDOOR_OBJS) -o out/hatred

out/hatredctl: $(CLIENT_OBJS)
	$(CXX) $(CXX_ARGS) $(CLIENT_OBJS) -o out/hatredctl

build/backdoor/main.o: src/backdoor/main.cc src/backdoor/net.h src/backdoor/util.h src/backdoor/upnp.h src/backdoor/http.h src/backdoor/ssdp.h src/backdoor/xml.h src/proto/proto.h
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

build/proto/proto.o: src/proto/proto.cc src/proto/proto.h
	mkdir -p build/proto/
	$(CXX) $(CXX_ARGS) -c src/proto/proto.cc -o build/proto/proto.o

build/client/main.o: src/client/main.cc src/proto/proto.h src/common/fnutil.h
	mkdir -p build/client/
	$(CXX) $(CXX_ARGS) -c src/client/main.cc -o build/client/main.o