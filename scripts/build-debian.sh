#!/bin/bash
# Build script for Debian/Ubuntu systems
# Simple RSync Daemon - Build Script

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

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

# Function to check if command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Function to detect Debian/Ubuntu version
detect_distro() {
    if [ -f /etc/os-release ]; then
        . /etc/os-release
        DISTRO_NAME="$NAME"
        DISTRO_VERSION="$VERSION_ID"
        print_status "Detected: $DISTRO_NAME $DISTRO_VERSION"
    else
        print_error "Could not detect distribution"
        exit 1
    fi
}

# Function to install dependencies
install_dependencies() {
    print_status "Installing build dependencies..."
    
    # Update package list
    sudo apt-get update
    
    # Install essential build tools
    sudo apt-get install -y \
        build-essential \
        cmake \
        pkg-config \
        git \
        wget \
        curl
    
    # Install development libraries
    sudo apt-get install -y \
        libssl-dev \
        libjsoncpp-dev \
        libpthread-stubs0-dev
    
    # Install additional tools (optional)
    sudo apt-get install -y \
        clang-format \
        cppcheck \
        valgrind \
        gdb \
        doxygen
    
    print_success "Dependencies installed successfully"
}

# Function to check dependencies
check_dependencies() {
    print_status "Checking build dependencies..."
    
    local missing_deps=()
    
    # Check required commands
    for cmd in g++ cmake pkg-config; do
        if ! command_exists "$cmd"; then
            missing_deps+=("$cmd")
        fi
    done
    
    # Check required libraries
    if ! pkg-config --exists openssl; then
        missing_deps+=("libssl-dev")
    fi
    
    if ! pkg-config --exists jsoncpp; then
        missing_deps+=("libjsoncpp-dev")
    fi
    
    if [ ${#missing_deps[@]} -ne 0 ]; then
        print_error "Missing dependencies: ${missing_deps[*]}"
        print_status "Installing missing dependencies..."
        install_dependencies
    else
        print_success "All dependencies are available"
    fi
}

# Function to build the project
build_project() {
    print_status "Building Simple RSync Daemon..."
    
    # Create build directory
    if [ -d "build" ]; then
        print_warning "Build directory already exists, cleaning..."
        rm -rf build
    fi
    
    mkdir -p build
    cd build
    
    # Configure with CMake
    print_status "Configuring with CMake..."
    cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DBUILD_TESTS=ON \
        -DBUILD_EXAMPLES=ON \
        -DENABLE_LOGGING=ON \
        -DENABLE_SSL=ON
    
    # Build
    print_status "Building project..."
    local cpu_count=$(nproc)
    make -j"$cpu_count"
    
    print_success "Build completed successfully"
}

# Function to run tests
run_tests() {
    print_status "Running tests..."
    
    if [ -f "Makefile" ]; then
        make test
        print_success "Tests completed"
    else
        print_warning "No tests found or build not completed"
    fi
}

# Function to install the project
install_project() {
    print_status "Installing Simple RSync Daemon..."
    
    if [ -f "Makefile" ]; then
        sudo make install
        print_success "Installation completed successfully"
        
        # Show installation info
        echo ""
        print_status "Installation Summary:"
        echo "  Binary: /usr/local/bin/simple-rsyncd-x86_64"
        echo "  Library: /usr/local/lib/libsimple-rsyncd.dylib"
        echo "  Headers: /usr/local/include/simple-rsyncd/"
        echo "  Config: /usr/local/etc/simple-rsyncd/"
        echo "  Docs: /usr/local/share/simple-rsyncd/docs/"
        
        # Test binary
        if command_exists simple-rsyncd-x86_64; then
            echo ""
            print_status "Testing binary..."
            simple-rsyncd-x86_64 --version
        fi
    else
        print_error "Build not completed, cannot install"
        exit 1
    fi
}

# Function to create package
create_package() {
    print_status "Creating package..."
    
    if [ -f "Makefile" ]; then
        # Check if cpack is available
        if command_exists cpack; then
            make package
            print_success "Package created successfully"
            
            # List created packages
            ls -la *.deb *.rpm 2>/dev/null || print_warning "No packages found"
        else
            print_warning "cpack not available, skipping package creation"
        fi
    else
        print_error "Build not completed, cannot create package"
        exit 1
    fi
}

# Function to show help
show_help() {
    echo "Usage: $0 [OPTIONS]"
    echo ""
    echo "Options:"
    echo "  -h, --help          Show this help message"
    echo "  -d, --deps          Install dependencies only"
    echo "  -b, --build         Build project only"
    echo "  -t, --test          Run tests only"
    echo "  -i, --install       Install project only"
    echo "  -p, --package       Create package only"
    echo "  -a, --all           Full build and install (default)"
    echo ""
    echo "Examples:"
    echo "  $0                  # Full build and install"
    echo "  $0 --deps           # Install dependencies only"
    echo "  $0 --build          # Build project only"
    echo "  $0 --install        # Install from existing build"
}

# Main function
main() {
    print_status "Simple RSync Daemon Build Script for Debian/Ubuntu"
    echo ""
    
    # Parse command line arguments
    local install_deps=false
    local build_project_flag=false
    local run_tests_flag=false
    local install_project_flag=false
    local create_package_flag=false
    local full_build=true
    
    while [[ $# -gt 0 ]]; do
        case $1 in
            -h|--help)
                show_help
                exit 0
                ;;
            -d|--deps)
                install_deps=true
                full_build=false
                shift
                ;;
            -b|--build)
                build_project_flag=true
                full_build=false
                shift
                ;;
            -t|--test)
                run_tests_flag=true
                full_build=false
                shift
                ;;
            -i|--install)
                install_project_flag=true
                full_build=false
                shift
                ;;
            -p|--package)
                create_package_flag=true
                full_build=false
                shift
                ;;
            -a|--all)
                full_build=true
                shift
                ;;
            *)
                print_error "Unknown option: $1"
                show_help
                exit 1
                ;;
        esac
    done
    
    # Detect distribution
    detect_distro
    
    # Check if running as root
    if [[ $EUID -eq 0 ]]; then
        print_error "This script should not be run as root"
        exit 1
    fi
    
    # Check if we're in the right directory
    if [ ! -f "CMakeLists.txt" ]; then
        print_error "CMakeLists.txt not found. Please run this script from the project root directory."
        exit 1
    fi
    
    # Execute requested actions
    if [ "$full_build" = true ]; then
        print_status "Performing full build and install..."
        check_dependencies
        build_project
        run_tests
        install_project
        create_package
    else
        if [ "$install_deps" = true ]; then
            install_dependencies
        fi
        
        if [ "$build_project_flag" = true ]; then
            check_dependencies
            build_project
        fi
        
        if [ "$run_tests_flag" = true ]; then
            run_tests
        fi
        
        if [ "$install_project_flag" = true ]; then
            install_project
        fi
        
        if [ "$create_package_flag" = true ]; then
            create_package
        fi
    fi
    
    print_success "Build script completed successfully!"
}

# Run main function with all arguments
main "$@"
