# Developer Setup Guide

Step-by-step guide to set up your development environment for Simple RSync Daemon.

## Prerequisites Checklist

- [ ] C++ compiler (GCC 7+ or Clang 8+)
- [ ] CMake 3.16+
- [ ] Git
- [ ] OpenSSL development libraries
- [ ] pkg-config

---

## Step 1: Install System Dependencies

### Linux (Ubuntu/Debian)

```bash
sudo apt update
sudo apt install -y \
    build-essential \
    cmake \
    git \
    libssl-dev \
    pkg-config
```

### macOS

```bash
brew install cmake openssl pkg-config
```

---

## Step 2: Clone Repository

```bash
git clone https://github.com/SimpleDaemons/simple-rsyncd.git
cd simple-rsyncd
```

---

## Step 3: Initial Build

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

---

**Last Updated:** December 2024
