FROM gcc:14-bookworm AS builder

# Install certificates and update package lists in a single layer
RUN apt-get update -y && \
    apt-get upgrade -y && \
    apt-get install --reinstall ca-certificates -y && \
    update-ca-certificates

# Install dependencies in a single layer to reduce image size
RUN apt-get update && apt-get install -y \
    cmake \
    git \
    libboost-dev \
    libboost-regex-dev \
    libasio-dev \
    libssl-dev \
    zlib1g-dev \
    libcurl4-openssl-dev \
    wget \
    && rm -rf /var/lib/apt/lists/*

# Set git configurations to improve clone reliability
RUN git config --global http.postBuffer 524288000 && \
    git config --global http.lowSpeedLimit 0 && \
    git config --global http.lowSpeedTime 999999

WORKDIR /deps

# Install Crow with retry mechanism
RUN for i in 1 2 3; do \
        git clone --depth 1 https://github.com/CrowCpp/Crow.git && break || \
        rm -rf Crow && sleep 5; \
    done && \
    cd Crow && \
    mkdir build && \
    cd build && \
    cmake .. \
        -DCROW_BUILD_EXAMPLES=OFF \
        -DCROW_BUILD_TESTS=OFF \
        -DCROW_ENABLE_SSL=ON && \
    make -j$(nproc) && \
    make install

# Install CPR with retry mechanism
RUN for i in 1 2 3; do \
        git clone --depth 1 https://github.com/libcpr/cpr.git && break || \
        rm -rf cpr && sleep 5; \
    done && \
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

# Download json.hpp with retry mechanism
RUN mkdir -p /tmp/json && \
    for i in 1 2 3; do \
        wget https://raw.githubusercontent.com/nlohmann/json/develop/single_include/nlohmann/json.hpp -O /tmp/json/json.hpp && break || \
        rm -f /tmp/json/json.hpp && sleep 5; \
    done

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

# Runtime stage
FROM ubuntu:latest

# Install runtime dependencies in a single layer
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