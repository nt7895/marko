### Base environment container ###
# Get the base Ubuntu image from Docker Hub
FROM ubuntu:noble as base

ARG DEBIAN_FRONTEND=noninteractive

# Update the base image and install build environment
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    curl \
    httpie \
    libboost-log-dev \
    libboost-regex-dev \
    libboost-system-dev \
    libgmock-dev \
    libgtest-dev \
    netcat-openbsd \
    poppler-utils \
    pkg-config \
    libjsoncpp-dev \
    sqlite3 \
    libsqlite3-dev \
    g++ cmake git curl lcov gcovr \
    && rm -rf /var/lib/apt/lists/*