FROM ghcr.io/xboxdev/nxdk-buildbase:git-39eb90d1

RUN apk add --no-cache git
RUN git clone --recursive https://github.com/xboxdev/nxdk.git /usr/src/nxdk
