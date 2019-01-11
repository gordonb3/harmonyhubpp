CC       = $(CROSS_COMPILE)g++
CFLAGS  +=  -I../../lib -I../../src -c -Wall $(EXTRAFLAGS)
LDFLAGS += -lstdc++ -lcurl -pthread -lboost_system
HHC_OBJ  = $(patsubst %.cpp,%.o,$(wildcard *.cpp))
DEPS     = $(patsubst %.cpp,%.o,$(wildcard ../../lib/jsoncpp/*.cpp)) $(patsubst %.cpp,%.o,$(wildcard ../../src/*.cpp))

harmony_monitor : $(HHC_OBJ) $(DEPS)
	$(CC) $(HHC_OBJ) $(DEPS) $(LDFLAGS) -o harmony_monitor

%.o: %.cpp
	$(CC) $(CFLAGS) $(EXTRAFLAGS) $< -o $@

distclean: clean

clean:
	rm -f $(HHC_OBJ) $(DEPS) harmony_monitor
