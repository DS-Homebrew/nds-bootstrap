FROM devkitpro/devkitarm

# lzss from https://github.com/ahezard/nds-bootstrap/issues/740#issuecomment-527304079
RUN wget https://cdn.discordapp.com/attachments/283769550611152897/615767904926826498/lzss && \
    mv lzss /bin && \
    chmod +x /bin/lzss

COPY . /nds-bootstrap
WORKDIR /nds-bootstrap
CMD ["make", "package-release"]
