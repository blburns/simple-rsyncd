#!/bin/bash

# Simple RSync Daemon - Docker Deployment Script
# Automated deployment and management

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Default values
PROFILE="runtime"
CONFIG_DIR="./config"
DATA_DIR="./data"
LOGS_DIR="./logs"
FORCE=false
CLEAN=false
BUILD=false
PULL=false
DETACH=true
FOLLOW_LOGS=false

# Help function
show_help() {
    cat << EOF
Simple RSync Daemon - Docker Deployment Script

Usage: $0 [OPTIONS]

Options:
    -p, --profile PROFILE     Deployment profile (dev, runtime, build)
                              Default: runtime
    -c, --config-dir DIR      Configuration directory
                              Default: ./config
    -d, --data-dir DIR        Data directory
                              Default: ./data
    -l, --logs-dir DIR        Logs directory
                              Default: ./logs
    -f, --force               Force deployment (stop existing containers)
    --clean                   Clean deployment (remove containers and volumes)
    --build                   Build images before deployment
    --pull                    Pull latest images before deployment
    --no-detach               Run in foreground (don't detach)
    --follow-logs             Follow logs after deployment
    -h, --help                Show this help message

Commands:
    start                     Start the service
    stop                      Stop the service
    restart                   Restart the service
    status                    Show service status
    logs                      Show service logs
    shell                     Access container shell
    test                      Test the deployment

Examples:
    $0                                    # Deploy runtime profile
    $0 -p dev                            # Deploy development profile
    $0 --build --force                   # Build and force deploy
    $0 --clean --build                   # Clean deployment with build
    $0 start                             # Start the service
    $0 stop                              # Stop the service
    $0 logs                              # Show logs
    $0 shell                             # Access container shell
    $0 test                              # Test deployment

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
COMMAND="deploy"
while [[ $# -gt 0 ]]; do
    case $1 in
        -p|--profile)
            PROFILE="$2"
            shift 2
            ;;
        -c|--config-dir)
            CONFIG_DIR="$2"
            shift 2
            ;;
        -d|--data-dir)
            DATA_DIR="$2"
            shift 2
            ;;
        -l|--logs-dir)
            LOGS_DIR="$2"
            shift 2
            ;;
        -f|--force)
            FORCE=true
            shift
            ;;
        --clean)
            CLEAN=true
            shift
            ;;
        --build)
            BUILD=true
            shift
            ;;
        --pull)
            PULL=true
            shift
            ;;
        --no-detach)
            DETACH=false
            shift
            ;;
        --follow-logs)
            FOLLOW_LOGS=true
            shift
            ;;
        start|stop|restart|status|logs|shell|test)
            COMMAND="$1"
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
    case $PROFILE in
        dev|runtime|build)
            ;;
        *)
            log_error "Invalid profile: $PROFILE"
            log_error "Valid options: dev, runtime, build"
            exit 1
            ;;
    esac
}

# Check prerequisites
check_prerequisites() {
    log_info "Checking prerequisites..."

    # Check if Docker is installed
    if ! command -v docker > /dev/null 2>&1; then
        log_error "Docker is not installed or not in PATH"
        exit 1
    fi

    # Check if Docker Compose is installed
    if ! command -v docker-compose > /dev/null 2>&1; then
        log_error "Docker Compose is not installed or not in PATH"
        exit 1
    fi

    # Check if Docker daemon is running
    if ! docker info > /dev/null 2>&1; then
        log_error "Docker daemon is not running"
        exit 1
    fi

    log_success "Prerequisites check passed"
}

# Create directories
create_directories() {
    log_info "Creating necessary directories..."

    mkdir -p "$CONFIG_DIR" "$DATA_DIR" "$LOGS_DIR"

    # Create default configuration if it doesn't exist
    if [ ! -f "$CONFIG_DIR/rsyncd.conf" ]; then
        log_info "Creating default configuration..."
        cp config/rsyncd.conf.example "$CONFIG_DIR/rsyncd.conf"
        log_success "Default configuration created at $CONFIG_DIR/rsyncd.conf"
    fi

    # Set proper permissions
    chmod 755 "$CONFIG_DIR" "$DATA_DIR" "$LOGS_DIR"

    log_success "Directories created successfully"
}

# Clean deployment
clean_deployment() {
    if [ "$CLEAN" = true ]; then
        log_info "Cleaning deployment..."

        # Stop and remove containers
        docker-compose down --volumes --remove-orphans 2>/dev/null || true

        # Remove images
        docker-compose down --rmi all 2>/dev/null || true

        # Clean up volumes
        docker volume prune -f 2>/dev/null || true

        log_success "Deployment cleaned"
    fi
}

# Build images
build_images() {
    if [ "$BUILD" = true ]; then
        log_info "Building Docker images..."

        if [ -f "scripts/build-docker.sh" ]; then
            ./scripts/build-docker.sh -d ubuntu
        else
            docker-compose build
        fi

        log_success "Images built successfully"
    fi
}

# Pull images
pull_images() {
    if [ "$PULL" = true ]; then
        log_info "Pulling latest images..."
        docker-compose pull
        log_success "Images pulled successfully"
    fi
}

# Deploy service
deploy_service() {
    log_info "Deploying simple-rsyncd with profile: $PROFILE"

    # Set environment variables
    export CONFIG_DIR="$CONFIG_DIR"
    export DATA_DIR="$DATA_DIR"
    export LOGS_DIR="$LOGS_DIR"

    # Force stop if requested
    if [ "$FORCE" = true ]; then
        log_info "Force stopping existing containers..."
        docker-compose down 2>/dev/null || true
    fi

    # Deploy based on profile
    case $PROFILE in
        dev)
            log_info "Starting development environment..."
            if [ "$DETACH" = true ]; then
                docker-compose --profile dev up -d dev
            else
                docker-compose --profile dev up dev
            fi
            ;;
        runtime)
            log_info "Starting runtime environment..."
            if [ "$DETACH" = true ]; then
                docker-compose --profile runtime up -d simple-rsyncd
            else
                docker-compose --profile runtime up simple-rsyncd
            fi
            ;;
        build)
            log_info "Starting build environment..."
            docker-compose --profile build up build-ubuntu
            ;;
    esac

    log_success "Deployment completed successfully"
}

# Start service
start_service() {
    log_info "Starting simple-rsyncd service..."

    case $PROFILE in
        dev)
            docker-compose --profile dev up -d dev
            ;;
        runtime)
            docker-compose --profile runtime up -d simple-rsyncd
            ;;
        *)
            docker-compose up -d
            ;;
    esac

    log_success "Service started successfully"
}

# Stop service
stop_service() {
    log_info "Stopping simple-rsyncd service..."
    docker-compose down
    log_success "Service stopped successfully"
}

# Restart service
restart_service() {
    log_info "Restarting simple-rsyncd service..."
    stop_service
    start_service
    log_success "Service restarted successfully"
}

# Show service status
show_status() {
    log_info "Service status:"
    docker-compose ps

    # Show health status
    log_info "Health status:"
    if docker-compose ps | grep -q "simple-rsyncd"; then
        HEALTH_STATUS=$(docker inspect --format='{{.State.Health.Status}}' simple-rsyncd 2>/dev/null || echo "unknown")
        log_info "Container health: $HEALTH_STATUS"
    fi

    # Show resource usage
    log_info "Resource usage:"
    docker stats --no-stream --format "table {{.Container}}\t{{.CPUPerc}}\t{{.MemUsage}}" $(docker-compose ps -q) 2>/dev/null || true
}

# Show logs
show_logs() {
    log_info "Showing service logs..."
    docker-compose logs -f
}

# Access container shell
access_shell() {
    log_info "Accessing container shell..."

    case $PROFILE in
        dev)
            docker-compose exec dev bash
            ;;
        runtime)
            docker-compose exec simple-rsyncd bash
            ;;
        *)
            # Try to find running container
            CONTAINER=$(docker-compose ps -q | head -1)
            if [ -n "$CONTAINER" ]; then
                docker exec -it "$CONTAINER" bash
            else
                log_error "No running containers found"
                exit 1
            fi
            ;;
    esac
}

# Test deployment
test_deployment() {
    log_info "Testing deployment..."

    # Check if service is running
    if ! docker-compose ps | grep -q "Up"; then
        log_error "Service is not running"
        exit 1
    fi

    # Test rsync connection
    log_info "Testing rsync connection..."
    if command -v rsync > /dev/null 2>&1; then
        if rsync rsync://localhost/ > /dev/null 2>&1; then
            log_success "Rsync connection test passed"
        else
            log_warning "Rsync connection test failed (this may be normal if no modules are configured)"
        fi
    else
        log_warning "rsync client not available for testing"
    fi

    # Test port connectivity
    log_info "Testing port connectivity..."
    if command -v nc > /dev/null 2>&1; then
        if nc -z localhost 873; then
            log_success "Port 873 connectivity test passed"
        else
            log_error "Port 873 connectivity test failed"
            exit 1
        fi
    else
        log_warning "netcat not available for port testing"
    fi

    # Show configuration
    log_info "Configuration:"
    docker-compose config

    log_success "Deployment test completed"
}

# Follow logs after deployment
follow_logs() {
    if [ "$FOLLOW_LOGS" = true ]; then
        log_info "Following logs (Ctrl+C to stop)..."
        sleep 2
        show_logs
    fi
}

# Main function
main() {
    log_info "Simple RSync Daemon - Docker Deployment"
    log_info "Command: $COMMAND"
    log_info "Profile: $PROFILE"
    log_info "Config Dir: $CONFIG_DIR"
    log_info "Data Dir: $DATA_DIR"
    log_info "Logs Dir: $LOGS_DIR"
    log_info "Force: $FORCE"
    log_info "Clean: $CLEAN"
    log_info "Build: $BUILD"
    log_info "Pull: $PULL"

    # Validate inputs
    validate_inputs

    # Change to script directory
    cd "$(dirname "$0")/.."

    # Check prerequisites
    check_prerequisites

    # Execute command
    case $COMMAND in
        deploy)
            create_directories
            clean_deployment
            build_images
            pull_images
            deploy_service
            follow_logs
            ;;
        start)
            start_service
            ;;
        stop)
            stop_service
            ;;
        restart)
            restart_service
            ;;
        status)
            show_status
            ;;
        logs)
            show_logs
            ;;
        shell)
            access_shell
            ;;
        test)
            test_deployment
            ;;
        *)
            log_error "Unknown command: $COMMAND"
            show_help
            exit 1
            ;;
    esac
}

# Run main function
main "$@"
