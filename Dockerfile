FROM ubuntu:20.04

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get -y update && apt-get -y upgrade && apt-get -y install git wget flex bison gperf python3 python3-pip python3-setuptools cmake ninja-build ccache libffi-dev libssl-dev dfu-util && update-alternatives --install /usr/bin/python python /usr/bin/python3 10

RUN mkdir /esp && cd /esp && git clone -b v4.2 --recursive https://github.com/espressif/esp-idf.git && cd esp-idf && ./install.sh

ENTRYPOINT ["bash", "-c", "source /esp/esp-idf/export.sh && \"$@\"", "-s"]
