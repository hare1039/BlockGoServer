CXX         = docker run --rm -v ${PWD}:/src hare1039/block-go-backend:latest
TARGET	    = app debug
OBJECT	    = 
DOCKER_ENV  = docker run -d --name blockgo-server --restart=unless-stopped -v ${PWD}:/work -p 9002:9002 alpine
DOCKER_ENV_LOG = docker logs -f blockgo-server
DOCKER_ENV_RM  = docker stop blockgo-server && docker rm blockgo-server
DOCKER_ENV_DBG = docker run --rm -v ${PWD}:/work -it --net=host --security-opt seccomp=unconfined --cap-add=SYS_PTRACE hare1039/ubuntu-gdb
DOCKER_MAKE = docker run --rm -v ${PWD}:/work hare1039/alpinemake
CXXFLAGS    = -Wall -Wextra
RELEASE_FLAGS = -O3 -DNDEBUG
DEBUG_FLAGS = -O0 -g

app: main.cpp game_ctrl.hpp websocket_server.hpp enable_spdlog.hpp
	$(CXX) -o $@ $(CXXFLAGS) $(RELEASE_FLAGS) $<

debug: main.cpp game_ctrl.hpp websocket_server.hpp enable_spdlog.hpp
	$(CXX) -o $@ $(CXXFLAGS) $(DEBUG_FLAGS) $<

.PHONY: main
main:
	$(DOCKER_MAKE) sh -c 'cd /work/ai-project/BlockGo && make BlockGoStatic;'

.PHONY: run
run: app main
	$(DOCKER_ENV) sh -c 'cd /work && ./app'

.PHONY: log
log:
	$(DOCKER_ENV_LOG)

.PHONY: clear-run
clear-run: 
	$(DOCKER_ENV_RM)

.PHONY: run-
run-: debug main
	$(DOCKER_ENV_DBG) sh -c 'cd /work && gdb debug'

.PHONY: rebuild
rebuild: 
	cd ./ai-project/BlockGo; \
	make clean BlockGoStatic; \
	bash -c './BlockGoStatic web <<< "11dddsssy2"';

.PHONY: clean
clean:
	rm -f $(TARGET) $(OBJECT)
