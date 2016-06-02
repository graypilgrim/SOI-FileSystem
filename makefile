CXX = g++
CXXFLAGS = -Wall -Werror --std=c++11 -pedantic
LINKFLAGS = --std=c++11

SOURCES = \
		FileSystem.cpp \
		main.cpp

OBJECTS=$(SOURCES:.cpp=.o)

fs: $(OBJECTS)
	$(CXX) -o $@ $^ $(LINKFLAGS)

%.o : %.cpp
	$(CXX) -c -o $@  $< $(CXXFLAGS)

clean:
	rm -f *.o
	rm fs

.PHONY:
	clean
