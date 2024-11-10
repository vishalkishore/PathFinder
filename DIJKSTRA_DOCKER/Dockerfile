# Use Ubuntu as base image
FROM ubuntu:22.04

# Avoid prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Install dependencies
RUN apt-get update && apt-get install -y \
    libcrypto++-dev \
    libssl-dev \
    libcpprest-dev \
    g++ \
    cmake \
    make \
    && rm -rf /var/lib/apt/lists/*

# Create app directory
WORKDIR /app

# Copy both backend and frontend directories
COPY backend/ /app/backend/
COPY frontend/ /app/frontend/

# Build the backend - with cleanup of any existing build directory
RUN cd /app/backend && \
    rm -rf build && \
    mkdir build && \
    cd build && \
    cmake .. && \
    make

# Expose port 8080
EXPOSE 8080

# Set the working directory to the build directory
WORKDIR /app/backend/build

# Command to run the server
CMD ["./spt_server"]
