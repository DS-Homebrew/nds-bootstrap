FROM ubuntu:18.04

RUN apt-get -y update && \
    apt-get -y install gpg \
                       libxml2 \
                       make \
                       wget \
                       xz-utils

RUN wget https://github.com/devkitPro/pacman/releases/latest/download/devkitpro-pacman.deb && \
    dpkg -i devkitpro-pacman.deb && \
    rm devkitpro-pacman.deb

# lzss from https://github.com/ahezard/nds-bootstrap/issues/740#issuecomment-527304079
RUN wget https://cdn.discordapp.com/attachments/283769550611152897/615767904926826498/lzss && \
    mv lzss /bin && \
    chmod +x /bin/lzss

RUN dkp-pacman -Sy && \
    dkp-pacman --noconfirm -S devkitARM \
                              dstools \
                              general-tools \
                              libfat-nds \  
                              libnds \
                              ndstool

ENV DEVKITPRO /opt/devkitpro
ENV DEVKITARM ${DEVKITPRO}/devkitARM
ENV PATH ${DEVKITPRO}/tools/bin:$PATH

COPY . /nds-bootstrap
WORKDIR /nds-bootstrap
CMD ["make", "package-release"]
