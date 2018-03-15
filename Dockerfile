FROM alpine:3.7
MAINTAINER hare1039 hare39393@hare1039.nctu.me

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

RUN echo 'cd /src && clang++ "$@" -I/header -lpthread -lboost_system -static -lPocoFoundation -lPocoNet -std=c++14' > /run.sh &&\
	chmod +x /run.sh

ENTRYPOINT ["sh", "/run.sh"]
