FROM alpine

LABEL MAINTAINER "Bradford Zhang"

ADD *.c /tmp/

ADD *.h /tmp/

ADD Makefile /tmp/

ADD scripts/build-docker-image.sh /tmp/

RUN chmod a+x /tmp/build-docker-image.sh && /tmp/build-docker-image.sh
