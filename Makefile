CXX         = docker run --rm -v ${PWD}:/src hare1039/block-go-backend:latest
TARGET	    = app
OBJECT	    = echo
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

echo: echo.cpp
	$(CXX) -o echo $^

.PHONY: run
run: all echo
	$(DOCKER_ENV) sh -c 'cd /work && ./app'
