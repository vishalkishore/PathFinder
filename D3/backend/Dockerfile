FROM ubuntu:20.04

ENV DEBIAN_FRONTEND=noninteractive

# Update and install dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    python3-pip \
    ninja-build \
    pkg-config \
    curl \
    && rm -rf /var/lib/apt/lists/*

# Install meson
RUN pip3 install meson

# Install Pistache
RUN git clone https://github.com/pistacheio/pistache.git \
    && cd pistache \
    && git submodule update --init --recursive \
    && meson setup build --prefix=/usr/local \
    && meson install -C build

WORKDIR /app

COPY . .

RUN mkdir -p build && cd build \
    && cmake .. \
    && make

EXPOSE 9080

CMD ["sh", "-c", "cd /app/build && ./server"]