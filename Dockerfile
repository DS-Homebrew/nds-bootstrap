FROM devkitpro/devkitarm

RUN apt-get -y update && \
    apt-get -y install gcc

COPY . /nds-bootstrap
WORKDIR /nds-bootstrap
RUN cd lzss && \
    make lzss && \
    cp lzss /bin/
CMD ["make", "package-release"]
