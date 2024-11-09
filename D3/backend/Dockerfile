FROM ubuntu:20.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update -y && \
    apt-get upgrade -y && \
    apt-get install --reinstall ca-certificates -y && \
    update-ca-certificates

# Update and install dependencies
RUN apt-get install -y \
    build-essential \
    cmake \
    git \
    python3-pip \
    ninja-build \
    pkg-config \
    nlohmann-json3-dev \
    curl \
    && rm -rf /var/lib/apt/lists/*

RUN git config --global http.postBuffer 524288000 && \
    git config --global http.lowSpeedLimit 0 && \
    git config --global http.lowSpeedTime 999999

# Install meson
RUN pip3 install meson

# Get number of CPU cores
RUN echo "nproc" > /get_cores.sh && chmod +x /get_cores.sh

# Install nlohmann-json using all cores
# RUN git clone https://github.com/nlohmann/json.git && \
#     cd json && \
#     mkdir build && \
#     cd build && \
#     cmake .. && \
#     make -j$(nproc) && \
#     make install && \
#     cd ../.. && \
#     rm -rf json

# Install Pistache using all cores
RUN git clone https://github.com/pistacheio/pistache.git \
    && cd pistache \
    && git submodule update --init --recursive \
    && meson setup build --prefix=/usr/local \
    && meson compile -C build -j $(nproc) \
    && meson install -C build

WORKDIR /app

COPY . .

# Build the application using all cores
RUN mkdir -p build && cd build \
    && cmake .. \
    && make -j$(nproc)

EXPOSE 9080

CMD ["sh", "-c", "cd /app/build && ./server"]