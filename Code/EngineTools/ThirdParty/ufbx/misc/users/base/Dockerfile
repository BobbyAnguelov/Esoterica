FROM ubuntu:latest

RUN apt-get update

WORKDIR setup

COPY 001-setup.sh .
RUN bash 001-setup.sh

ENV PATH /root/.cargo/bin:$PATH
