CXX         = docker run -v ${PWD}:/src hare1039/block-go-backend:0.0.8
TARGET	    = app
OBJECT	    = 
OBJECT_HTML = 
CXXFLAGS    = 


all: main.cpp
	$(CXX) -o $(TARGET) $(CXXFLAGS) $^

.PHONY: clean
clean:
	rm -f $(TARGET) $(OBJECT)

.PHONY: run
run: all
	./app
