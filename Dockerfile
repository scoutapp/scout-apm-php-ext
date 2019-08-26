FROM php:7.3-zts

RUN mkdir /e
COPY tests /e/tests
COPY config.m4 /e/config.m4
COPY zend_scoutapm.c /e/zend_scoutapm.c
COPY zend_scoutapm.h /e/zend_scoutapm.h

RUN cd /e/ \
    && phpize \
    && ./configure --enable-scoutapm \
    && make test

ENTRYPOINT []
CMD ["cp", "-v", "/e/modules/scoutapm.so", "/v/scoutapm-20180731-zts.so"]
