#!/bin/bash

# simple-rsyncd macOS Build Script
# This script builds the simple-rsyncd application for macOS

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Script configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build"
DIST_DIR="$PROJECT_ROOT/dist"
VERSION="0.1.0"

# Build options
BUILD_TYPE="Release"
BUILD_SHARED_LIBS="ON"
BUILD_TESTS="ON"
BUILD_EXAMPLES="OFF"
ENABLE_LOGGING="ON"
ENABLE_SSL="ON"
USE_SYSTEM_LIBS="OFF"
PACKAGE="false"

# Function to print colored output
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Function to show usage
show_usage() {
    echo "Usage: $0 [OPTIONS]"
    echo ""
    echo "Options:"
    echo "  --help, -h           Show this help message"
    echo "  --debug              Build in debug mode"
    echo "  --release            Build in release mode (default)"
    echo "  --tests              Build with tests (default)"
    echo "  --no-tests           Build without tests"
    echo "  --examples           Build with examples"
    echo "  --no-examples        Build without examples (default)"
    echo "  --ssl                Enable SSL support (default)"
    echo "  --no-ssl             Disable SSL support"
    echo "  --system-libs        Use system libraries"
    echo "  --homebrew-libs      Use Homebrew libraries (default)"
    echo "  --package            Create macOS package after build"
    echo "  --clean              Clean build directory before building"
    echo "  --install            Install after building"
    echo ""
    echo "Examples:"
    echo "  $0                    # Build in release mode with tests"
    echo "  $0 --debug            # Build in debug mode"
    echo "  $0 --package          # Build and create package"
    echo "  $0 --clean --install  # Clean build, build, and install"
}

# Function to parse command line arguments
parse_arguments() {
    while [[ $# -gt 0 ]]; do
        case $1 in
            --help|-h)
                show_usage
                exit 0
                ;;
            --debug)
                BUILD_TYPE="Debug"
                shift
                ;;
            --release)
                BUILD_TYPE="Release"
                shift
                ;;
            --tests)
                BUILD_TESTS="ON"
                shift
                ;;
            --no-tests)
                BUILD_TESTS="OFF"
                shift
                ;;
            --examples)
                BUILD_EXAMPLES="ON"
                shift
                ;;
            --no-examples)
                BUILD_EXAMPLES="OFF"
                shift
                ;;
            --ssl)
                ENABLE_SSL="ON"
                shift
                ;;
            --no-ssl)
                ENABLE_SSL="OFF"
                shift
                ;;
            --system-libs)
                USE_SYSTEM_LIBS="ON"
                shift
                ;;
            --homebrew-libs)
                USE_SYSTEM_LIBS="OFF"
                shift
                ;;
            --package)
                PACKAGE="true"
                shift
                ;;
            --clean)
                CLEAN_BUILD="true"
                shift
                ;;
            --install)
                INSTALL="true"
                shift
                ;;
            *)
                print_error "Unknown option: $1"
                show_usage
                exit 1
                ;;
        esac
    done
}

# Function to check dependencies
check_dependencies() {
    print_status "Checking dependencies..."
    
    # Check for required tools
    local missing_tools=()
    
    if ! command -v cmake &> /dev/null; then
        missing_tools+=("cmake")
    fi
    
    if ! command -v make &> /dev/null; then
        missing_tools+=("make")
    fi
    
    if ! command -v clang++ &> /dev/null; then
        missing_tools+=("clang++")
    fi
    
    if [[ ${#missing_tools[@]} -gt 0 ]]; then
        print_error "Missing required tools: ${missing_tools[*]}"
        print_status "Please install missing tools:"
        print_status "  brew install cmake"
        exit 1
    fi
    
    # Check for required libraries
    local missing_libs=()
    
    if [[ "$ENABLE_SSL" == "ON" ]]; then
        if [[ "$USE_SYSTEM_LIBS" == "OFF" ]]; then
            if ! brew list openssl &> /dev/null; then
                missing_libs+=("openssl")
            fi
        fi
    fi
    
    if ! brew list jsoncpp &> /dev/null; then
        missing_libs+=("jsoncpp")
    fi
    
    if [[ ${#missing_libs[@]} -gt 0 ]]; then
        print_status "Installing missing libraries..."
        for lib in "${missing_libs[@]}"; do
            brew install "$lib"
        done
    fi
    
    print_success "All dependencies satisfied"
}

# Function to clean build directory
clean_build() {
    if [[ "$CLEAN_BUILD" == "true" ]]; then
        print_status "Cleaning build directory..."
        if [[ -d "$BUILD_DIR" ]]; then
            rm -rf "$BUILD_DIR"
        fi
        print_success "Build directory cleaned"
    fi
}

# Function to create build directory
create_build_dir() {
    print_status "Creating build directory..."
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
}

# Function to configure build
configure_build() {
    print_status "Configuring build..."
    
    local cmake_args=(
        "-DCMAKE_BUILD_TYPE=$BUILD_TYPE"
        "-DBUILD_SHARED_LIBS=$BUILD_SHARED_LIBS"
        "-DBUILD_TESTS=$BUILD_TESTS"
        "-DBUILD_EXAMPLES=$BUILD_EXAMPLES"
        "-DENABLE_LOGGING=$ENABLE_LOGGING"
        "-DENABLE_SSL=$ENABLE_SSL"
        "-DUSE_SYSTEM_LIBS=$USE_SYSTEM_LIBS"
        "-DCMAKE_OSX_DEPLOYMENT_TARGET=12.0"
    )
    
    # Add architecture flags
    local arch_flags=()
    if [[ "$(uname -m)" == "arm64" ]]; then
        arch_flags+=("arm64")
    elif [[ "$(uname -m)" == "x86_64" ]]; then
        arch_flags+=("x86_64")
    fi
    
    if [[ ${#arch_flags[@]} -gt 0 ]]; then
        cmake_args+=("-DCMAKE_OSX_ARCHITECTURES=${arch_flags[*]}")
    fi
    
    cmake "${cmake_args[@]}" "$PROJECT_ROOT"
    
    print_success "Build configured"
}

# Function to build project
build_project() {
    print_status "Building project..."
    
    local num_cores=$(sysctl -n hw.ncpu)
    make -j"$num_cores"
    
    print_success "Build completed"
}

# Function to run tests
run_tests() {
    if [[ "$BUILD_TESTS" == "ON" ]]; then
        print_status "Running tests..."
        make test
        print_success "Tests completed"
    fi
}

# Function to create package
create_package() {
    if [[ "$PACKAGE" == "true" ]]; then
        print_status "Creating macOS package..."
        make package
        print_success "Package created"
    fi
}

# Function to install
install_project() {
    if [[ "$INSTALL" == "true" ]]; then
        print_status "Installing project..."
        sudo make install
        print_success "Installation completed"
    fi
}

# Function to show build summary
show_build_summary() {
    print_success "Build completed successfully!"
    echo ""
    echo "Build Summary:"
    echo "  Build Type: $BUILD_TYPE"
    echo "  Build Directory: $BUILD_DIR"
    echo "  Tests: $BUILD_TESTS"
    echo "  Examples: $BUILD_EXAMPLES"
    echo "  SSL: $ENABLE_SSL"
    echo "  System Libraries: $USE_SYSTEM_LIBS"
    echo ""
    
    if [[ "$PACKAGE" == "true" ]]; then
        echo "Package created in: $BUILD_DIR"
    fi
    
    if [[ "$INSTALL" == "true" ]]; then
        echo "Project installed to: /usr/local"
    fi
}

# Main function
main() {
    print_status "Starting simple-rsyncd macOS build..."
    print_status "Version: $VERSION"
    print_status "Build Type: $BUILD_TYPE"
    print_status "Tests: $BUILD_TESTS"
    print_status "Examples: $BUILD_EXAMPLES"
    print_status "SSL: $ENABLE_SSL"
    print_status "System Libraries: $USE_SYSTEM_LIBS"
    echo ""
    
    # Parse command line arguments
    parse_arguments "$@"
    
    # Check dependencies
    check_dependencies
    
    # Clean build if requested
    clean_build
    
    # Create build directory
    create_build_dir
    
    # Configure build
    configure_build
    
    # Build project
    build_project
    
    # Run tests
    run_tests
    
    # Create package if requested
    create_package
    
    # Install if requested
    install_project
    
    # Show build summary
    show_build_summary
}

# Run main function with all arguments
main "$@"
