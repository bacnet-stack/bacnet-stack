# This dockerfile is used to build debian packages
# it should not be invoked directly.
# To build a debian package, run:
#
#   make deb
#
# The debian package will be copied into the working
# directory. You can change the version by modifying
# the version in the Makefile.
#
FROM ubuntu:14.04

RUN apt-get -qq update

RUN \
  export DEBIAN_FRONTEND=noninteractive && \
  apt-get -qq install -qq -y ruby ruby-dev ruby-bundler > /dev/null && \
  apt-get -qq install -qq -y build-essential rpm > /dev/null && \
  rm -rf /var/lib/apt/lists/*

RUN gem install fpm -q > /dev/null

WORKDIR /data

RUN mkdir -p /data/build/usr/local/bin /data/build/var/lib/aptfile

COPY bin/aptfile /data/build/usr/local/bin/aptfile

RUN echo "VERSION" > /data/build/var/lib/aptfile/VERSION \
    && chmod +x /data/build/usr/local/bin/aptfile

RUN fpm --log warn \
        -s dir \
        -t deb \
        -C /data/build \
        --name aptfile \
        --version "VERSION" \
        --description "a simple method of defining apt-get dependencies for an application" \
        --maintainer "SeatGeek <hi@seatgeek.com>" \
        --vendor "SeatGeek" \
        --license "BSD 3-Clause" \
        --url "https://github.com/seatgeek/bash-aptfile" \
        --deb-no-default-config-files \
        .

RUN dpkg -i /data/aptfile_"VERSION"_amd64.deb && \
    dpkg -s aptfile && \
    aptfile -v
