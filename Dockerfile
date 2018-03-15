FROM alpine:3.7
MAINTAINER hare1039 hare39393@hare1039.nctu.me

RUN apk update && \
    apk add --no-cache curl git boost-dev clang musl-dev g++ && \
    git clone --depth 1 https://github.com/zaphoyd/websocketpp.git /websocketpp && \
	curl -L -o /websocketpp/json.hpp https://github.com/nlohmann/json/releases/download/v3.1.2/json.hpp

RUN cat /usr/include/sys/poll.h | grep -v warning > /usr/include/sys/poll.hh &&\
	mv /usr/include/sys/poll.h /usr/include/sys/poll.h.old &&\
	mv /usr/include/sys/poll.hh /usr/include/sys/poll.h

RUN echo 'cd /src && clang++ "$@" -I/websocketpp -lboost_system -static -std=c++14' > /run.sh &&\
	chmod +x /run.sh

ENTRYPOINT ["sh", "/run.sh"]
