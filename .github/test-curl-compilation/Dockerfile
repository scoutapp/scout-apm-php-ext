FROM ubuntu:22.04
RUN apt-get update && apt-get -y upgrade \
  && apt-get -y install software-properties-common \
  && add-apt-repository ppa:ondrej/php \
  && apt-get update \
  && DEBIAN_FRONTEND=noninteractive apt-get -y install php8.0 php8.0-xml php8.0-dev

ARG WITH_CURL_EXT=no
RUN if [ "$WITH_CURL_EXT" = "yes" ]; then apt-get -y install php8.0-curl; fi

ARG WITH_LIBCURL=no
RUN if [ "$WITH_LIBCURL" = "yes" ]; then apt-get -y install libcurl4-openssl-dev; fi

ADD *.c *.h config.m4 /scoutapm/

RUN cd /scoutapm/ \
    && ls -l \
    && phpize \
    && ./configure --enable-scoutapm \
    && cat config.h \
    && make \
    && make install \
    && echo "zend_extension=scoutapm" > /etc/php/8.0/mods-available/scoutapm.ini \
    && phpenmod scoutapm

ADD .github/test-curl-compilation/analyse-curl.php /scoutapm/analyse-curl.php

ENTRYPOINT ["/usr/bin/php"]
CMD ["/scoutapm/analyse-curl.php"]
