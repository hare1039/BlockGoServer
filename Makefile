CXX         = docker run --rm -v ${PWD}:/src hare1039/block-go-backend:latest
TARGET	    = app
OBJECT	    = 
DOCKER_ENV  = docker run --rm -v ${PWD}:/work -it -p 9002:9002 alpine
DOCKER_MAKE = docker run --rm -v ${PWD}:/work hare1039/alpinemake
CXXFLAGS    = -g


app: main.cpp
	$(CXX) -o $(TARGET) $(CXXFLAGS) $^

.PHONY: debug
debug: main.cpp
	$(CXX) -o $(TARGET) $(CXXFLAGS) -g $^

.PHONY: main
main:
	$(DOCKER_MAKE) sh -c 'cd /work/ai-project/BlockGo && make BlockGoStatic;'

.PHONY: run
run: app main
	$(DOCKER_ENV) sh -c 'cd /work && ./app'

.PHONY: clean
clean:
	rm -f $(TARGET) $(OBJECT)
