CXX         = docker run --rm -v ${PWD}:/src hare1039/block-go-backend:latest
TARGET	    = app
OBJECT	    = 
DOCKER_ENV  = docker run --rm -v ${PWD}:/work -p 9002:9002 alpine
DOCKER_ENV_DBG = docker run --rm -v ${PWD}:/work -it --net=host --security-opt seccomp=unconfined --cap-add=SYS_PTRACE hare1039/ubuntu-gdb
DOCKER_MAKE = docker run --rm -v ${PWD}:/work hare1039/alpinemake
CXXFLAGS    = -Wall -Wextra


app: main.cpp game_ctrl.hpp websocket_server.hpp
	$(CXX) -o $(TARGET) $(CXXFLAGS) $<

.PHONY: debug
debug: main.cpp game_ctrl.hpp websocket_server.hpp
	$(CXX) -o $(TARGET) $(CXXFLAGS) -g $<

.PHONY: main
main:
	$(DOCKER_MAKE) sh -c 'cd /work/ai-project/BlockGo && make BlockGoStatic;'

.PHONY: run
run: app main
	$(DOCKER_ENV) sh -c 'cd /work && ./app'

.PHONY: run-
run-: app main
	$(DOCKER_ENV_DBG) sh -c 'cd /work && gdb app'

.PHONY: rebuild
rebuild: 
	cd ./ai-project/BlockGo; \
	make clean BlockGoStatic; \
	bash -c './BlockGoStatic web <<< "11dddsssy2"';

.PHONY: clean
clean:
	rm -f $(TARGET) $(OBJECT)
