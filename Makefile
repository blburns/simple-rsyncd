# Makefile for simple-rsyncd
# Simple RSync Daemon - A lightweight and secure rsync server

# Variables
PROJECT_NAME = simple-rsyncd
VERSION = 0.1.0
BUILD_DIR = build
DIST_DIR = dist
PACKAGE_DIR = packaging

# Platform detection
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    PLATFORM = macos
    CXX = clang++
    CXXFLAGS = -std=c++17 -Wall -Wextra -Wpedantic -O2 -DNDEBUG
    LDFLAGS = -lssl -lcrypto
    # macOS specific flags
    CXXFLAGS += -target x86_64-apple-macos12.0 -target arm64-apple-macos12.0
    # Detect processor cores for parallel builds
    PARALLEL_JOBS = $(shell sysctl -n hw.ncpu)
    # macOS install paths
    INSTALL_PREFIX = /usr/local
    CONFIG_DIR = $(INSTALL_PREFIX)/etc/$(PROJECT_NAME)
else
    PLATFORM = linux
    CXX = g++
    CXXFLAGS = -std=c++17 -Wall -Wextra -Wpedantic -O2 -DNDEBUG
    LDFLAGS = -lssl -lcrypto -lpthread
    # Linux specific flags
    PARALLEL_JOBS = $(shell nproc)
    # Linux install paths
    INSTALL_PREFIX = /usr/local
    CONFIG_DIR = /etc/$(PROJECT_NAME)
endif

# Directories
SRC_DIR = src
INCLUDE_DIR = include
CONFIG_DIR_SRC = config
SCRIPTS_DIR = scripts
DEPLOYMENT_DIR = deployment

# Default target
all: build

# Create build directory
$(BUILD_DIR)-dir:
	mkdir -p $(BUILD_DIR)

# Build using CMake
build: $(BUILD_DIR)-dir
	cd $(BUILD_DIR) && cmake .. && make -j$(PARALLEL_JOBS)

# Clean build
clean:
	rm -rf $(BUILD_DIR)
	rm -rf $(DIST_DIR)

# Install
install: build
	cd $(BUILD_DIR) && sudo make install

# Uninstall
uninstall:
	sudo rm -f $(INSTALL_PREFIX)/bin/$(PROJECT_NAME)
	sudo rm -f $(INSTALL_PREFIX)/lib/lib$(PROJECT_NAME).so
	sudo rm -f $(INSTALL_PREFIX)/lib/lib$(PROJECT_NAME).dylib
	sudo rm -rf $(INSTALL_PREFIX)/include/$(PROJECT_NAME)
	sudo rm -rf $(CONFIG_DIR)

# Test
test: build
	cd $(BUILD_DIR) && make test

# Package RPM (Linux only)
package-rpm: build
ifeq ($(PLATFORM),linux)
	@echo "Building RPM package..."
	@mkdir -p $(DIST_DIR)
	cd $(BUILD_DIR) && cpack -G RPM
	mv $(BUILD_DIR)/$(PROJECT_NAME)-$(VERSION)-*.rpm $(DIST_DIR)/
else
	@echo "RPM packages are only supported on Linux"
endif

# Package DEB (Linux only)
package-deb: build
ifeq ($(PLATFORM),linux)
	@echo "Building DEB package..."
	@mkdir -p $(DIST_DIR)
	cd $(BUILD_DIR) && cpack -G DEB
	mv $(BUILD_DIR)/$(PROJECT_NAME)-$(VERSION)-*.deb $(DIST_DIR)/
else
	@echo "DEB packages are only supported on Linux"
endif

# Package using build scripts (recommended)
package-script:
ifeq ($(PLATFORM),macos)
	@echo "Building macOS package with build script..."
	./scripts/build-macos.sh --package
else ifeq ($(PLATFORM),linux)
	@echo "Building Linux package with build script..."
	./scripts/build-linux.sh --package
else
	@echo "Package generation not supported on this platform"
endif

# Package all formats
package-all: package-rpm package-deb package-script

# Development targets
dev-build: $(BUILD_DIR)-dir
	cd $(BUILD_DIR) && cmake .. -DCMAKE_BUILD_TYPE=Debug && make -j$(PARALLEL_JOBS)

dev-test: dev-build
	cd $(BUILD_DIR) && make test

# Documentation
docs: build
	cd $(BUILD_DIR) && make docs

# Static analysis
analyze: $(BUILD_DIR)-dir
	cd $(BUILD_DIR) && cmake .. -DCMAKE_BUILD_TYPE=Debug -DENABLE_ANALYZE=ON && make -j$(PARALLEL_JOBS)

# Coverage
coverage: $(BUILD_DIR)-dir
	cd $(BUILD_DIR) && cmake .. -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON && make -j$(PARALLEL_JOBS)
	cd $(BUILD_DIR) && make test
	cd $(BUILD_DIR) && lcov --capture --directory . --output-file coverage.info
	cd $(BUILD_DIR) && genhtml coverage.info --output-directory coverage_report

# Format code
format:
	find $(SRC_DIR) $(INCLUDE_DIR) -name "*.cpp" -o -name "*.hpp" | xargs clang-format -i

# Check code style
check-style:
	find $(SRC_DIR) $(INCLUDE_DIR) -name "*.cpp" -o -name "*.hpp" | xargs clang-format --dry-run --Werror

# Lint code
lint: check-style
	cppcheck --enable=all --std=c++17 $(SRC_DIR) $(INCLUDE_DIR)

# Security scan
security-scan:
	bandit -r $(SRC_DIR)
	semgrep --config=auto $(SRC_DIR)

# Dependencies
deps:
ifeq ($(PLATFORM),macos)
	brew install openssl jsoncpp cmake
else ifeq ($(PLATFORM),linux)
	sudo apt-get update
	sudo apt-get install -y build-essential cmake libssl-dev libjsoncpp-dev
	# For RPM-based systems
	# sudo yum install -y gcc-c++ cmake openssl-devel jsoncpp-devel
endif

# Docker targets
docker-build:
	docker build -t $(PROJECT_NAME):$(VERSION) .

docker-run:
	docker run -d --name $(PROJECT_NAME)-$(VERSION) -p 873:873 $(PROJECT_NAME):$(VERSION)

docker-stop:
	docker stop $(PROJECT_NAME)-$(VERSION)
	docker rm $(PROJECT_NAME)-$(VERSION)

# Service management
service-install: install
ifeq ($(PLATFORM),macos)
	sudo cp $(DEPLOYMENT_DIR)/launchd/com.$(PROJECT_NAME).$(PROJECT_NAME).plist /Library/LaunchDaemons/
	sudo launchctl load /Library/LaunchDaemons/com.$(PROJECT_NAME).$(PROJECT_NAME).plist
else ifeq ($(PLATFORM),linux)
	sudo cp $(DEPLOYMENT_DIR)/systemd/$(PROJECT_NAME).service /etc/systemd/system/
	sudo systemctl daemon-reload
	sudo systemctl enable $(PROJECT_NAME)
	sudo systemctl start $(PROJECT_NAME)
endif

service-uninstall:
ifeq ($(PLATFORM),macos)
	sudo launchctl unload /Library/LaunchDaemons/com.$(PROJECT_NAME).$(PROJECT_NAME).plist
	sudo rm -f /Library/LaunchDaemons/com.$(PROJECT_NAME).$(PROJECT_NAME).plist
else ifeq ($(PLATFORM),linux)
	sudo systemctl stop $(PROJECT_NAME)
	sudo systemctl disable $(PROJECT_NAME)
	sudo rm -f /etc/systemd/system/$(PROJECT_NAME).service
	sudo systemctl daemon-reload
endif

service-status:
ifeq ($(PLATFORM),macos)
	launchctl list | grep $(PROJECT_NAME)
else ifeq ($(PLATFORM),linux)
	sudo systemctl status $(PROJECT_NAME)
endif

# Configuration management
config-install: install
	sudo mkdir -p $(CONFIG_DIR)
	sudo cp -r $(CONFIG_DIR_SRC)/* $(CONFIG_DIR)/
	sudo chown -R root:root $(CONFIG_DIR)
	sudo chmod -R 644 $(CONFIG_DIR)
	sudo find $(CONFIG_DIR) -type d -exec chmod 755 {} \;

config-backup:
	@mkdir -p $(DIST_DIR)/config-backup
	cp -r $(CONFIG_DIR_SRC) $(DIST_DIR)/config-backup/
	tar -czf $(DIST_DIR)/config-backup-$(VERSION).tar.gz -C $(DIST_DIR) config-backup

# Log management
log-rotate: install
ifeq ($(PLATFORM),linux)
	sudo cp $(DEPLOYMENT_DIR)/logrotate.d/$(PROJECT_NAME) /etc/logrotate.d/
	sudo chmod 644 /etc/logrotate.d/$(PROJECT_NAME)
endif

# Backup and restore
backup: config-backup
	@mkdir -p $(DIST_DIR)/backup
	tar -czf $(DIST_DIR)/$(PROJECT_NAME)-backup-$(VERSION).tar.gz \
		$(CONFIG_DIR_SRC) \
		$(DEPLOYMENT_DIR) \
		$(SRC_DIR) \
		$(INCLUDE_DIR) \
		CMakeLists.txt \
		Makefile \
		README.md \
		LICENSE

restore: backup
	@echo "Restoring from backup..."
	@if [ -f $(DIST_DIR)/$(PROJECT_NAME)-backup-$(VERSION).tar.gz ]; then \
		tar -xzf $(DIST_DIR)/$(PROJECT_NAME)-backup-$(VERSION).tar.gz; \
		echo "Restore completed"; \
	else \
		echo "No backup found at $(DIST_DIR)/$(PROJECT_NAME)-backup-$(VERSION).tar.gz"; \
	fi

# Cleanup
distclean: clean
	rm -rf $(DIST_DIR)
	rm -rf $(PACKAGE_DIR)
	find . -name "*.o" -delete
	find . -name "*.a" -delete
	find . -name "*.so" -delete
	find . -name "*.dylib" -delete
	find . -name "*.exe" -delete
	find . -name "*.dll" -delete

# Help
help:
	@echo "Available targets:"
	@echo "  all              - Build the project (default)"
	@echo "  build            - Build using CMake"
	@echo "  clean            - Clean build files"
	@echo "  install          - Install the project"
	@echo "  uninstall        - Uninstall the project"
	@echo "  test             - Run tests"
	@echo "  package-rpm      - Build RPM package (Linux only)"
	@echo "  package-deb      - Build DEB package (Linux only)"
	@echo "  package-script   - Build package using platform script"
	@echo "  package-all      - Build all package formats"
	@echo "  dev-build        - Build in debug mode"
	@echo "  dev-test         - Run tests in debug mode"
	@echo "  docs             - Build documentation"
	@echo "  analyze          - Run static analysis"
	@echo "  coverage         - Generate coverage report"
	@echo "  format           - Format source code"
	@echo "  check-style      - Check code style"
	@echo "  lint             - Run linting tools"
	@echo "  security-scan    - Run security scanning tools"
	@echo "  deps             - Install dependencies"
	@echo "  docker-build     - Build Docker image"
	@echo "  docker-run       - Run Docker container"
	@echo "  docker-stop      - Stop Docker container"
	@echo "  service-install  - Install system service"
	@echo "  service-uninstall- Uninstall system service"
	@echo "  service-status   - Check service status"
	@echo "  config-install   - Install configuration files"
	@echo "  config-backup    - Backup configuration"
	@echo "  log-rotate       - Install log rotation"
	@echo "  backup           - Create full backup"
	@echo "  restore          - Restore from backup"
	@echo "  distclean        - Clean all generated files"
	@echo "  help             - Show this help message"

# Phony targets
.PHONY: all build clean install uninstall test package-rpm package-deb package-script package-all \
        dev-build dev-test docs analyze coverage format check-style lint security-scan deps \
        docker-build docker-run docker-stop service-install service-uninstall service-status \
        config-install config-backup log-rotate backup restore distclean help

# Default target
.DEFAULT_GOAL := all
