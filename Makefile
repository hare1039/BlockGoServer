CXX         = docker run --rm -v ${PWD}:/src hare1039/block-go-backend:latest
TARGET	    = app
OBJECT	    = 
OBJECT_HTML = 
CXXFLAGS    = 


all: main.cpp
	$(CXX) -o $(TARGET) $(CXXFLAGS) $^

.PHONY: debug
debug: main.cpp
	$(CXX) -o $(TARGET) $(CXXFLAGS) -g $^

.PHONY: clean
clean:
	rm -f $(TARGET) $(OBJECT)

.PHONY: run
run: all
	./app
