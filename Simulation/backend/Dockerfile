# Dockerfile
FROM gcc:14-bookworm AS builder

RUN apt-get update -y
RUN apt-get upgrade -y
RUN apt-get install --reinstall ca-certificates -y

# Install dependencies
RUN apt-get update && apt-get install -y \
    cmake \
    git \
    libboost-dev \
    libboost-regex-dev \
    libasio-dev \
    libssl-dev \
    zlib1g-dev \
    libcurl4-openssl-dev \
    && rm -rf /var/lib/apt/lists/*

# Install Crow
WORKDIR /deps
RUN git clone https://github.com/CrowCpp/Crow.git && \
    cd Crow && \
    mkdir build && \
    cd build && \
    cmake .. \
        -DCROW_BUILD_EXAMPLES=OFF \
        -DCROW_BUILD_TESTS=OFF \
        -DCROW_ENABLE_SSL=ON && \
    make -j$(nproc) && \
    make install

# Install CPR with tests and examples disabled
RUN git clone https://github.com/libcpr/cpr.git && \
    cd cpr && \
    mkdir build && \
    cd build && \
    cmake .. \
        -DCPR_USE_SYSTEM_CURL=ON \
        -DCPR_BUILD_TESTS=OFF \
        -DCPR_BUILD_TESTS_SSL=OFF \
        -DCPR_BUILD_EXAMPLES=OFF \
        -DCMAKE_INSTALL_PREFIX=/usr \
        -DCPR_FORCE_USE_SYSTEM_CURL=ON && \
    make -j$(nproc) && \
    make install && \
    ln -s /usr/lib/cmake/cpr /usr/share/cmake/cpr

# Download latest json.hpp if not present in include folder
RUN mkdir -p /tmp/json && \
    wget https://raw.githubusercontent.com/nlohmann/json/develop/single_include/nlohmann/json.hpp -O /tmp/json/json.hpp

# Set up build directory
WORKDIR /app
COPY CMakeLists.txt .
COPY src/ src/
COPY include/ include/

# Copy json.hpp if it doesn't exist in include
RUN if [ ! -f include/json.hpp ]; then cp /tmp/json/json.hpp include/; fi

# Build the application
RUN mkdir -p build && \
    cd build && \
    cmake .. \
        -DCMAKE_PREFIX_PATH="/usr/lib/cmake/cpr" \
        -DCMAKE_INSTALL_PREFIX=/usr/local && \
    make && \
    make install

# Create minimal runtime image
FROM ubuntu:latest

# Install runtime dependencies
RUN apt-get update && apt-get install -y \
    libboost-regex1.74.0 \
    libssl3 \
    zlib1g \
    libcurl4 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy the executable from builder stage
COPY --from=builder /usr/local/bin/main /main

EXPOSE 8080

ENTRYPOINT ["/main"]