CXX         = docker run --rm -v ${PWD}:/src hare1039/block-go-backend:latest
TARGET	    = app
OBJECT	    = 
DOCKER_ENV  = docker run --rm -v ${PWD}:/work -p 9002:9002 alpine
DOCKER_ENV_DBG = docker run --rm -v ${PWD}:/work -it --net=host --security-opt seccomp=unconfined --cap-add=SYS_PTRACE hare1039/alpine-gdb
DOCKER_MAKE = docker run --rm -v ${PWD}:/work hare1039/alpinemake
CXXFLAGS    = -Wall -Wextra


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

.PHONY: run-
run-: main
	$(DOCKER_ENV_DBG) sh -c 'cd /work && gdb app'

.PHONY: clean
clean:
	rm -f $(TARGET) $(OBJECT)
