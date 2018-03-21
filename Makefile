CXX         = docker run --rm -v ${PWD}:/src hare1039/block-go-backend:latest
TARGET	    = app
OBJECT	    = 
DOCKER_ENV  = docker run -it -v ${PWD}:/work --rm -p 9002:9002 alpine
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
	$(DOCKER_ENV) sh -c 'cd /work && ./app'
