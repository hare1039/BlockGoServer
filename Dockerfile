FROM alpine:3.7
MAINTAINER hare1039 hare1039@hare1039.nctu.me

RUN echo 'http://dl-4.alpinelinux.org/alpine/edge/testing' >> /etc/apk/repositories

RUN apk update && \
    apk add --no-cache curl git boost-dev clang musl-dev g++ make linux-headers && \
    git clone --depth 1 https://github.com/zaphoyd/websocketpp.git /header && \
	curl -L -o /header/json.hpp https://github.com/nlohmann/json/releases/download/v3.1.2/json.hpp && \
	wget https://github.com/pocoproject/poco/archive/poco-1.9.0-release.tar.gz && \
	tar xzf poco-1.9.0-release.tar.gz && \
	cd poco-poco-1.9.0-release && \
	./configure --config=Linux-clang --minimal --static --no-tests --no-samples && \
	make -j4 && \
	make -s install && \
	cd .. && \
	rm -rf poco*

RUN cat /usr/include/sys/poll.h | grep -v warning > /usr/include/sys/poll.hh &&\
	mv /usr/include/sys/poll.h /usr/include/sys/poll.h.old &&\
	mv /usr/include/sys/poll.hh /usr/include/sys/poll.h

RUN wget https://github.com/gabime/spdlog/archive/v0.16.3.zip &&\
    unzip v0.16.3.zip &&\
    mv spdlog-0.16.3/include/spdlog /header

RUN echo 'cd /src && clang++ -static -std=c++14 "$@" -I/header -pthread -lboost_log_setup-mt -lboost_log-mt -lboost_thread-mt -lboost_system-mt -lPocoFoundation -lPocoNet' > /run.sh &&\
	chmod +x /run.sh

ENTRYPOINT ["sh", "/run.sh"]
