#!/bin/bash

# Simple RSync Daemon - Docker Build Script
# Cross-platform Docker build automation

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Default values
DISTRO="ubuntu"
ARCH="x86_64"
CLEAN=false
PUSH=false
REGISTRY=""
TAG="latest"
PLATFORMS="linux/amd64,linux/arm64,linux/arm/v7"

# Help function
show_help() {
    cat << EOF
Simple RSync Daemon - Docker Build Script

Usage: $0 [OPTIONS]

Options:
    -d, --distro DISTRO     Target distribution (ubuntu, centos, alpine, all)
                           Default: ubuntu
    -a, --arch ARCH        Target architecture (x86_64, arm64, armv7, all)
                           Default: x86_64
    -t, --tag TAG          Docker image tag
                           Default: latest
    -r, --registry REGISTRY Docker registry URL
    -p, --platforms PLATFORMS Comma-separated list of platforms for multi-arch
                           Default: linux/amd64,linux/arm64,linux/arm/v7
    -c, --clean            Clean build cache before building
    --push                 Push images to registry after building
    --no-cache             Build without using cache
    -h, --help             Show this help message

Examples:
    $0                                    # Build Ubuntu x86_64 image
    $0 -d centos                         # Build CentOS image
    $0 -d all                            # Build all distributions
    $0 -a all                            # Build all architectures
    $0 -d ubuntu -a arm64                # Build Ubuntu ARM64 image
    $0 --clean --push                    # Clean build and push to registry
    $0 -r myregistry.com -t v1.0.0       # Build and tag for custom registry

EOF
}

# Logging functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -d|--distro)
            DISTRO="$2"
            shift 2
            ;;
        -a|--arch)
            ARCH="$2"
            shift 2
            ;;
        -t|--tag)
            TAG="$2"
            shift 2
            ;;
        -r|--registry)
            REGISTRY="$2"
            shift 2
            ;;
        -p|--platforms)
            PLATFORMS="$2"
            shift 2
            ;;
        -c|--clean)
            CLEAN=true
            shift
            ;;
        --push)
            PUSH=true
            shift
            ;;
        --no-cache)
            NO_CACHE="--no-cache"
            shift
            ;;
        -h|--help)
            show_help
            exit 0
            ;;
        *)
            log_error "Unknown option: $1"
            show_help
            exit 1
            ;;
    esac
done

# Validate inputs
validate_inputs() {
    case $DISTRO in
        ubuntu|centos|alpine|all)
            ;;
        *)
            log_error "Invalid distribution: $DISTRO"
            log_error "Valid options: ubuntu, centos, alpine, all"
            exit 1
            ;;
    esac

    case $ARCH in
        x86_64|arm64|armv7|all)
            ;;
        *)
            log_error "Invalid architecture: $ARCH"
            log_error "Valid options: x86_64, arm64, armv7, all"
            exit 1
            ;;
    esac
}

# Clean build cache
clean_cache() {
    if [ "$CLEAN" = true ]; then
        log_info "Cleaning Docker build cache..."
        docker builder prune -f
        docker system prune -f
        log_success "Build cache cleaned"
    fi
}

# Build single distribution
build_distro() {
    local distro=$1
    local arch=$2
    local target=""
    local image_name="simple-rsyncd"

    # Set target based on distribution
    case $distro in
        ubuntu)
            target="ubuntu-builder"
            ;;
        centos)
            target="centos-builder"
            ;;
        alpine)
            target="alpine-builder"
            ;;
    esac

    # Set image name with registry and tag
    if [ -n "$REGISTRY" ]; then
        image_name="${REGISTRY}/${image_name}"
    fi
    image_name="${image_name}:${TAG}-${distro}-${arch}"

    log_info "Building $distro $arch image..."
    log_info "Target: $target"
    log_info "Image: $image_name"

    # Build the image
    if docker-compose build $NO_CACHE --build-arg TARGETPLATFORM="linux/${arch}" build-${distro}; then
        log_success "Successfully built $distro $arch image"

        # Tag the image
        docker tag "simple-rsyncd-build-${distro}_build-${distro}" "$image_name"
        log_success "Tagged image as $image_name"

        # Push if requested
        if [ "$PUSH" = true ]; then
            log_info "Pushing $image_name to registry..."
            if docker push "$image_name"; then
                log_success "Successfully pushed $image_name"
            else
                log_error "Failed to push $image_name"
                exit 1
            fi
        fi
    else
        log_error "Failed to build $distro $arch image"
        exit 1
    fi
}

# Build runtime image
build_runtime() {
    local distro=$1
    local arch=$2
    local image_name="simple-rsyncd"

    # Set image name with registry and tag
    if [ -n "$REGISTRY" ]; then
        image_name="${REGISTRY}/${image_name}"
    fi
    image_name="${image_name}:${TAG}"

    log_info "Building runtime image for $distro $arch..."
    log_info "Image: $image_name"

    # Build runtime image
    if docker-compose build $NO_CACHE --build-arg TARGETPLATFORM="linux/${arch}" simple-rsyncd; then
        log_success "Successfully built runtime image"

        # Tag the image
        docker tag "simple-rsyncd_simple-rsyncd" "$image_name"
        log_success "Tagged runtime image as $image_name"

        # Push if requested
        if [ "$PUSH" = true ]; then
            log_info "Pushing $image_name to registry..."
            if docker push "$image_name"; then
                log_success "Successfully pushed $image_name"
            else
                log_error "Failed to push $image_name"
                exit 1
            fi
        fi
    else
        log_error "Failed to build runtime image"
        exit 1
    fi
}

# Build multi-architecture image
build_multiarch() {
    local image_name="simple-rsyncd"

    # Set image name with registry and tag
    if [ -n "$REGISTRY" ]; then
        image_name="${REGISTRY}/${image_name}"
    fi
    image_name="${image_name}:${TAG}"

    log_info "Building multi-architecture image..."
    log_info "Platforms: $PLATFORMS"
    log_info "Image: $image_name"

    # Check if buildx is available
    if ! docker buildx version > /dev/null 2>&1; then
        log_error "Docker buildx is not available. Please install Docker buildx."
        exit 1
    fi

    # Create buildx builder if it doesn't exist
    if ! docker buildx inspect rsyncd-builder > /dev/null 2>&1; then
        log_info "Creating buildx builder..."
        docker buildx create --name rsyncd-builder --use
    else
        docker buildx use rsyncd-builder
    fi

    # Build multi-architecture image
    if docker buildx build \
        --platform "$PLATFORMS" \
        --tag "$image_name" \
        $NO_CACHE \
        $([ "$PUSH" = true ] && echo "--push" || echo "--load") \
        .; then
        log_success "Successfully built multi-architecture image"

        if [ "$PUSH" = true ]; then
            log_success "Successfully pushed multi-architecture image"
        fi
    else
        log_error "Failed to build multi-architecture image"
        exit 1
    fi
}

# Main build function
main() {
    log_info "Starting Docker build process..."
    log_info "Distribution: $DISTRO"
    log_info "Architecture: $ARCH"
    log_info "Tag: $TAG"
    log_info "Registry: ${REGISTRY:-'none'}"
    log_info "Push: $PUSH"
    log_info "Clean: $CLEAN"

    # Validate inputs
    validate_inputs

    # Clean cache if requested
    clean_cache

    # Change to script directory
    cd "$(dirname "$0")/.."

    # Build based on parameters
    if [ "$DISTRO" = "all" ] && [ "$ARCH" = "all" ]; then
        # Build multi-architecture image
        build_multiarch
    elif [ "$DISTRO" = "all" ]; then
        # Build all distributions for specific architecture
        for distro in ubuntu centos alpine; do
            build_distro "$distro" "$ARCH"
        done
        build_runtime "ubuntu" "$ARCH"
    elif [ "$ARCH" = "all" ]; then
        # Build all architectures for specific distribution
        for arch in x86_64 arm64 armv7; do
            build_distro "$DISTRO" "$arch"
        done
        build_runtime "$DISTRO" "x86_64"
    else
        # Build specific distribution and architecture
        build_distro "$DISTRO" "$ARCH"
        build_runtime "$DISTRO" "$ARCH"
    fi

    log_success "Docker build process completed successfully!"

    # Show built images
    log_info "Built images:"
    docker images | grep simple-rsyncd || true
}

# Run main function
main "$@"
