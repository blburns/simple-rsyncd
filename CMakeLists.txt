# CMakeLists.txt template for SimpleDaemons projects
# Simple RSync Daemon - A lightweight and secure rsync server
# Copyright 2024 SimpleDaemons

cmake_minimum_required(VERSION 3.16)
project(simple-rsyncd VERSION 0.3.1 LANGUAGES CXX)

include(GNUInstallDirs)
if(UNIX AND NOT APPLE AND NOT CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
    set(CMAKE_INSTALL_PREFIX "/usr" CACHE PATH "Installation prefix" FORCE)
endif()

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Platform detection
if(WIN32)
    set(PLATFORM_NAME "Windows")
elseif(APPLE)
    set(PLATFORM_NAME "macOS")
elseif(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
    set(PLATFORM_NAME "FreeBSD")
elseif(UNIX)
    set(PLATFORM_NAME "Linux")
endif()

# Build options
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(ENABLE_TESTS "Enable tests" ON)
option(ENABLE_PACKAGING "Enable package generation" ON)
option(ENABLE_SSL "Enable SSL/TLS support" ON)
option(ENABLE_JSON "Enable JSON support" ON)
option(ENABLE_YAML "Enable YAML support" ON)
option(ENABLE_STATIC_LINKING "Enable static linking for self-contained binaries" OFF)

# Find required packages
find_package(Threads REQUIRED)

if(ENABLE_SSL)
    if(APPLE)
        foreach(_openssl_hint IN ITEMS
                "/opt/homebrew/opt/openssl@3"
                "/opt/homebrew/opt/openssl"
                "/usr/local/opt/openssl@3"
                "/usr/local/opt/openssl")
            if(EXISTS "${_openssl_hint}")
                list(PREPEND CMAKE_PREFIX_PATH "${_openssl_hint}")
            endif()
        endforeach()
    endif()
    find_package(OpenSSL REQUIRED COMPONENTS SSL Crypto)
    # Older FindOpenSSL / some macOS setups set OPENSSL_* vars but not imported targets
    if(NOT TARGET OpenSSL::SSL)
        add_library(OpenSSL::SSL UNKNOWN IMPORTED)
        set_target_properties(OpenSSL::SSL PROPERTIES
            IMPORTED_LINK_INTERFACE_LANGUAGES "C"
            IMPORTED_LOCATION "${OPENSSL_SSL_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${OPENSSL_INCLUDE_DIR}")
    endif()
    if(NOT TARGET OpenSSL::Crypto)
        add_library(OpenSSL::Crypto UNKNOWN IMPORTED)
        set_target_properties(OpenSSL::Crypto PROPERTIES
            IMPORTED_LINK_INTERFACE_LANGUAGES "C"
            IMPORTED_LOCATION "${OPENSSL_CRYPTO_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${OPENSSL_INCLUDE_DIR}")
    endif()
endif()

if(ENABLE_JSON)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(JSONCPP REQUIRED jsoncpp)
endif()

if(ENABLE_YAML)
    find_package(yaml-cpp QUIET)
    if(NOT yaml-cpp_FOUND)
        # Try to find via pkg-config
        pkg_check_modules(YAMLCPP yaml-cpp)
        if(YAMLCPP_FOUND)
            set(yaml-cpp_FOUND TRUE)
        endif()
    endif()
    if(NOT yaml-cpp_FOUND)
        message(WARNING "yaml-cpp not found. YAML configuration support will be disabled.")
        set(ENABLE_YAML OFF)
    endif()
endif()

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Source files - separate main and tests from library sources
file(GLOB_RECURSE ALL_SOURCES "src/*.cpp" "src/*.c")
file(GLOB_RECURSE HEADERS "include/*.hpp" "include/*.h")

# Separate main.cpp and test files from library sources
set(LIBRARY_SOURCES ${ALL_SOURCES})
list(FILTER LIBRARY_SOURCES EXCLUDE REGEX ".*main\\.cpp$")
list(FILTER LIBRARY_SOURCES EXCLUDE REGEX ".*test.*\\.cpp$")
set(MAIN_SOURCE "")
file(GLOB MAIN_SOURCE "src/main.cpp")

# Create library target for shared code (used by executable and tests)
add_library(${PROJECT_NAME}_lib STATIC ${LIBRARY_SOURCES} ${HEADERS})

# Create executable that links to the library
if(MAIN_SOURCE)
    add_executable(${PROJECT_NAME} ${MAIN_SOURCE})
    target_link_libraries(${PROJECT_NAME} ${PROJECT_NAME}_lib)
else()
    # Fallback: if no main.cpp, create executable from all sources
    add_executable(${PROJECT_NAME} ${ALL_SOURCES} ${HEADERS})
endif()

# Link libraries to library target (shared by executable and tests)
target_link_libraries(${PROJECT_NAME}_lib PUBLIC Threads::Threads)

if(ENABLE_SSL)
    target_link_libraries(${PROJECT_NAME}_lib PUBLIC OpenSSL::SSL OpenSSL::Crypto)
endif()

if(ENABLE_JSON)
    set(JSONCPP_LINK_LIB "")
    if(ENABLE_STATIC_LINKING)
        find_library(JSONCPP_LINK_LIB NAMES libjsoncpp.a jsoncpp
            HINTS ${JSONCPP_LIBRARY_DIRS}
            PATHS /opt/local/lib /usr/local/lib /usr/lib)
    else()
        find_library(JSONCPP_LINK_LIB NAMES jsoncpp libjsoncpp
            HINTS ${JSONCPP_LIBRARY_DIRS}
            PATHS /opt/local/lib /usr/local/lib /usr/lib)
    endif()
    if(NOT JSONCPP_LINK_LIB)
        set(JSONCPP_LINK_LIB ${JSONCPP_LIBRARIES})
    endif()

    target_link_libraries(${PROJECT_NAME}_lib PUBLIC ${JSONCPP_LINK_LIB})
    target_link_directories(${PROJECT_NAME}_lib PUBLIC ${JSONCPP_LIBRARY_DIRS})
    target_include_directories(${PROJECT_NAME}_lib PUBLIC ${JSONCPP_INCLUDE_DIRS})
    target_compile_options(${PROJECT_NAME}_lib PRIVATE ${JSONCPP_CFLAGS_OTHER})
endif()

if(ENABLE_YAML AND yaml-cpp_FOUND)
    if(TARGET yaml-cpp::yaml-cpp)
        # yaml-cpp >= 0.8 exports a namespaced target
        target_link_libraries(${PROJECT_NAME}_lib PUBLIC yaml-cpp::yaml-cpp)
    elseif(TARGET yaml-cpp)
        target_link_libraries(${PROJECT_NAME}_lib PUBLIC yaml-cpp)
    else()
        if(ENABLE_STATIC_LINKING)
            find_library(YAMLCPP_STATIC_LIB yaml-cpp PATHS /opt/local/lib /usr/local/lib /usr/lib)
            if(YAMLCPP_STATIC_LIB)
                target_link_libraries(${PROJECT_NAME}_lib PUBLIC ${YAMLCPP_STATIC_LIB})
            else()
                target_link_libraries(${PROJECT_NAME}_lib PUBLIC ${YAMLCPP_LIBRARIES})
            endif()
        else()
            target_link_libraries(${PROJECT_NAME}_lib PUBLIC ${YAMLCPP_LIBRARIES})
        endif()
        target_include_directories(${PROJECT_NAME}_lib PRIVATE ${YAMLCPP_INCLUDE_DIRS})
        target_compile_options(${PROJECT_NAME}_lib PRIVATE ${YAMLCPP_CFLAGS_OTHER})
        target_link_directories(${PROJECT_NAME}_lib PRIVATE ${YAMLCPP_LIBRARY_DIRS})
    endif()
    target_compile_definitions(${PROJECT_NAME}_lib PRIVATE YAMLCPP_AVAILABLE=1)
endif()

# Note: the executable is already linked to ${PROJECT_NAME}_lib where it is created above.

# Compiler-specific options (apply to both library and executable)
if(MSVC)
    target_compile_options(${PROJECT_NAME}_lib PRIVATE /W3 /WX)
    if(MAIN_SOURCE)
        target_compile_options(${PROJECT_NAME} PRIVATE /W3 /WX)
    endif()
    if(ENABLE_STATIC_LINKING)
        target_compile_options(${PROJECT_NAME}_lib PRIVATE /MT)
        if(MAIN_SOURCE)
            target_compile_options(${PROJECT_NAME} PRIVATE /MT)
        endif()
    endif()
else()
    set(_common_warning_flags -Wall -Wextra -Wpedantic -Wno-unused-parameter)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        list(APPEND _common_warning_flags -Wno-unused-private-field)
    endif()
    target_compile_options(${PROJECT_NAME}_lib PRIVATE ${_common_warning_flags})
    if(MAIN_SOURCE)
        target_compile_options(${PROJECT_NAME} PRIVATE ${_common_warning_flags})
    endif()
    if(ENABLE_STATIC_LINKING)
        # Only apply static linking flags for GCC, not clang
        if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
            target_compile_options(${PROJECT_NAME}_lib PRIVATE -static-libgcc -static-libstdc++)
            target_link_options(${PROJECT_NAME}_lib PRIVATE -static-libgcc -static-libstdc++)
            if(MAIN_SOURCE)
                target_compile_options(${PROJECT_NAME} PRIVATE -static-libgcc -static-libstdc++)
                target_link_options(${PROJECT_NAME} PRIVATE -static-libgcc -static-libstdc++)
            endif()
        endif()
    endif()
endif()

# Static linking configuration
if(ENABLE_STATIC_LINKING)
    set(BUILD_SHARED_LIBS OFF)
    if(ENABLE_SSL)
        # Force static linking for OpenSSL
        set(OPENSSL_USE_STATIC_LIBS TRUE)
    endif()
    if(ENABLE_JSON)
        # Force static linking for jsoncpp
        set(JSONCPP_USE_STATIC_LIBS TRUE)
    endif()
endif()

# Install targets
if(APPLE)
    install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
    )
    install(DIRECTORY include/
        DESTINATION include
        FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
    )
    install(DIRECTORY config/
        DESTINATION /etc/${PROJECT_NAME}
        FILES_MATCHING
            PATTERN "*.conf"
            PATTERN "*.example"
            PATTERN "*.yaml"
            PATTERN "*.json"
            PATTERN "README.md"
    )
    install(FILES deployment/launchd/com.simpledaemons.${PROJECT_NAME}.plist
        DESTINATION /Library/LaunchDaemons
    )
else()
    install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
    install(DIRECTORY include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
    )
    install(DIRECTORY config/
        DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/${PROJECT_NAME}
        FILES_MATCHING PATTERN "*.conf" PATTERN "*.example"
    )
    if(UNIX)
        install(FILES deployment/systemd/${PROJECT_NAME}.service
            DESTINATION lib/systemd/system
        )
    endif()
endif()

# Tests
if(ENABLE_TESTS)
    enable_testing()
    add_subdirectory(src/tests)
endif()

# Package generation
if(ENABLE_PACKAGING)
    if(NOT PROJECT_DESCRIPTION)
        set(PROJECT_DESCRIPTION "Simple RSync Daemon - A lightweight and secure rsync server")
    endif()

    # Package information
    set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
    set(CPACK_PACKAGE_VENDOR "SimpleDaemons")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
    set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
    set(CPACK_PACKAGE_CONTACT "info@simpledaemons.com")
    set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/simpledaemons/${PROJECT_NAME}")

    # Package files — productbuild requires .txt/.html resources on macOS
    if(APPLE)
        set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/packaging/LICENSE.txt")
        set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/packaging/assets/readme.html")
        set(CPACK_RESOURCE_FILE_WELCOME "${CMAKE_CURRENT_SOURCE_DIR}/packaging/assets/welcome.html")
    else()
        set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
        set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
    endif()

    # Platform-specific package settings
    if(WIN32)
        # Windows MSI and ZIP packages
        set(CPACK_GENERATOR "WIX;ZIP")

        # WIX (MSI) settings
        set(CPACK_WIX_PRODUCT_GUID "12345678-1234-1234-1234-123456789012")
        set(CPACK_WIX_UPGRADE_GUID "87654321-4321-4321-4321-210987654321")
        set(CPACK_WIX_PROPERTY_ARPPRODUCTICON "${CMAKE_CURRENT_SOURCE_DIR}/assets/icon.ico")
        set(CPACK_WIX_PROPERTY_ARPHELPLINK "https://github.com/simpledaemons/${PROJECT_NAME}")
        set(CPACK_WIX_PROPERTY_ARPURLINFOABOUT "https://github.com/simpledaemons/${PROJECT_NAME}")
        set(CPACK_WIX_PROPERTY_ARPNOREPAIR "1")
        set(CPACK_WIX_PROPERTY_ARPNOMODIFY "1")

        # ZIP package settings
        set(CPACK_ZIP_COMPONENT_INSTALL ON)

    elseif(APPLE)
        # macOS DMG and PKG packages
        set(CPACK_GENERATOR "DragNDrop;productbuild")
        set(CPACK_PACKAGING_INSTALL_PREFIX "/usr/local")
        set(CPACK_SET_DESTDIR TRUE)
        set(CPACK_PRODUCTBUILD_DOMAINS TRUE)
        set(CPACK_PRODUCTBUILD_DOMAINS_ANYWHERE TRUE)
        set(CPACK_PRODUCTBUILD_DOMAINS_ROOT TRUE)
        set(CPACK_PACKAGE_RELOCATABLE FALSE)

        # DMG settings
        set(CPACK_DMG_VOLUME_NAME "${PROJECT_NAME}")
        set(CPACK_DMG_FORMAT "UDBZ")
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets/dmg_background.png")
            set(CPACK_DMG_BACKGROUND_IMAGE "${CMAKE_CURRENT_SOURCE_DIR}/assets/dmg_background.png")
        endif()
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets/DS_Store")
            set(CPACK_DMG_DS_STORE "${CMAKE_CURRENT_SOURCE_DIR}/assets/DS_Store")
        endif()

        # PKG settings
        set(CPACK_PRODUCTBUILD_IDENTIFIER "com.simpledaemons.${PROJECT_NAME}")
        set(CPACK_PRODUCTBUILD_PACKAGE_NAME "${PROJECT_NAME}")

    elseif(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
        set(CPACK_GENERATOR "FREEBSD")
        set(CPACK_PACKAGING_INSTALL_PREFIX "/")
        set(CPACK_SET_DESTDIR TRUE)
        set(CPACK_FREEBSD_PACKAGE_MAINTAINER "SimpleDaemons <info@simpledaemons.com>")
        set(CPACK_FREEBSD_PACKAGE_ORIGIN "net/${PROJECT_NAME}")
        set(CPACK_FREEBSD_PACKAGE_LICENSE "APACHE20")
        if(ENABLE_STATIC_LINKING)
            set(CPACK_FREEBSD_PACKAGE_RUNTIME_DEPENDS "")
        else()
            set(CPACK_FREEBSD_PACKAGE_RUNTIME_DEPENDS "openssl jsoncpp")
        endif()

    else()
        # Linux DEB and RPM packages
        set(CPACK_GENERATOR "DEB;RPM")
        set(CPACK_PACKAGING_INSTALL_PREFIX "/")
        set(CPACK_SET_DESTDIR TRUE)

        # DEB package settings
        set(CPACK_DEBIAN_PACKAGE_MAINTAINER "SimpleDaemons <info@simpledaemons.com>")
        set(CPACK_DEBIAN_PACKAGE_SECTION "net")
        set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
        set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
        if(ENABLE_STATIC_LINKING)
            set(CPACK_DEBIAN_PACKAGE_DEPENDS "")
        else()
            set(CPACK_DEBIAN_PACKAGE_DEPENDS "libssl3 (>= 3.0.0), libjsoncpp25 | libjsoncpp24 | libjsoncpp1")
        endif()
        set(CPACK_DEBIAN_PACKAGE_RECOMMENDS "systemd")
        set(CPACK_DEBIAN_PACKAGE_SUGGESTS "logrotate")
        set(CPACK_DEBIAN_PACKAGE_CONFLICTS "ntp")
        set(CPACK_DEBIAN_PACKAGE_PROVIDES "${PROJECT_NAME}")
        set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/simpledaemons/${PROJECT_NAME}")
        set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "${PROJECT_DESCRIPTION}")
        set(CPACK_DEBIAN_PACKAGE_LONG_DESCRIPTION "${PROJECT_DESCRIPTION}")

        # RPM package settings
        set(CPACK_RPM_PACKAGE_LICENSE "Apache-2.0")
        set(CPACK_RPM_PACKAGE_GROUP "Applications/System")
        set(CPACK_RPM_PACKAGE_URL "https://github.com/simpledaemons/${PROJECT_NAME}")
        set(CPACK_RPM_PACKAGE_DESCRIPTION "${PROJECT_DESCRIPTION}")
        if(ENABLE_STATIC_LINKING)
            set(CPACK_RPM_PACKAGE_REQUIRES "")
        else()
            set(CPACK_RPM_PACKAGE_REQUIRES "openssl-libs, jsoncpp")
        endif()
        set(CPACK_RPM_PACKAGE_SUGGESTS "systemd")
        set(CPACK_RPM_PACKAGE_CONFLICTS "ntp")
        set(CPACK_RPM_PACKAGE_PROVIDES "${PROJECT_NAME}")
    endif()

    # Set package architecture and determine architecture string
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|amd64")
        set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
        set(CPACK_RPM_PACKAGE_ARCHITECTURE "x86_64")
        set(PACKAGE_ARCH "amd64")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
        set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "arm64")
        set(CPACK_RPM_PACKAGE_ARCHITECTURE "aarch64")
        set(PACKAGE_ARCH "arm64")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "armv7l")
        set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "armhf")
        set(CPACK_RPM_PACKAGE_ARCHITECTURE "armv7hl")
        set(PACKAGE_ARCH "armhf")
    else()
        set(PACKAGE_ARCH "${CMAKE_SYSTEM_PROCESSOR}")
    endif()

    # Determine distribution/platform identifier for package naming
    if(APPLE)
        if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
            set(PLATFORM_ID "macos-apple")
        else()
            set(PLATFORM_ID "macos-intel")
        endif()
    elseif(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
        set(PLATFORM_ID "freebsd")
    elseif(UNIX)
        set(PLATFORM_ID "linux-generic")
        if(EXISTS "/etc/os-release")
            file(READ "/etc/os-release" OS_RELEASE)
            if(OS_RELEASE MATCHES "ID=([^\n]+)")
                string(REGEX REPLACE ".*ID=([^\n]+).*" "\\1" DISTRO_ID "${OS_RELEASE}")
                string(STRIP "${DISTRO_ID}" DISTRO_ID)
                string(TOLOWER "${DISTRO_ID}" DISTRO_ID)
                string(REPLACE "\"" "" DISTRO_ID "${DISTRO_ID}")
                if(DISTRO_ID MATCHES "debian|ubuntu")
                    set(PLATFORM_ID "linux-debian")
                endif()
            endif()
        endif()
    else()
        set(PLATFORM_ID "${PLATFORM_NAME}")
    endif()

    # Set package file names with standardized format: {project}-{version}-{platform}-{arch}.{ext}
    if(APPLE)
        set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-${PLATFORM_ID}")
    elseif(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
        set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-${PLATFORM_ID}-${PACKAGE_ARCH}")
        set(CPACK_FREEBSD_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-${PLATFORM_ID}-${PACKAGE_ARCH}")
    elseif(UNIX)
        set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-${PLATFORM_ID}-${PACKAGE_ARCH}")
        set(CPACK_DEBIAN_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-${PLATFORM_ID}-${PACKAGE_ARCH}")
        set(CPACK_RPM_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-${PLATFORM_ID}-${PACKAGE_ARCH}")
    else()
        set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-${PLATFORM_NAME}-${PACKAGE_ARCH}")
    endif()

    set(CPACK_PACKAGE_INSTALL_DIRECTORY "${PROJECT_NAME}")

    # Component installation
    set(CPACK_COMPONENT_APPLICATIONS_DISPLAY_NAME "Application")
    set(CPACK_COMPONENT_APPLICATIONS_DESCRIPTION "The ${PROJECT_NAME} executable and related files")

    set(CPACK_COMPONENT_DEVELOPMENT_DISPLAY_NAME "Development")
    set(CPACK_COMPONENT_DEVELOPMENT_DESCRIPTION "Header files for developing with ${PROJECT_NAME}")

    set(CPACK_COMPONENT_CONFIG_DISPLAY_NAME "Configuration")
    set(CPACK_COMPONENT_CONFIG_DESCRIPTION "Configuration files and examples")

    set(CPACK_COMPONENT_SERVICE_DISPLAY_NAME "Service")
    set(CPACK_COMPONENT_SERVICE_DESCRIPTION "System service files")

    # Set component dependencies
    set(CPACK_COMPONENT_DEVELOPMENT_DEPENDS applications)
    set(CPACK_COMPONENT_CONFIG_DEPENDS applications)
    set(CPACK_COMPONENT_SERVICE_DEPENDS applications)

    # Source package generation
    set(CPACK_SOURCE_GENERATOR "TGZ;ZIP")
    set(CPACK_SOURCE_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-src")
    set(CPACK_SOURCE_IGNORE_FILES
        "/build/"
        "/dist/"
        "/.git/"
        "/.gitignore"
        "/.vscode/"
        "/.vs/"
        "/CMakeCache.txt"
        "/CMakeFiles/"
        "/cmake_install.cmake"
        "/Makefile"
        "/install_manifest.txt"
        "/compile_commands.json"
        "/CTestTestfile.cmake"
        "/_CPack_Packages/"
        "/.DS_Store"
        "/Thumbs.db"
        "*.o"
        "*.a"
        "*.so"
        "*.dylib"
        "*.dll"
        "*.exe"
        "*.pdb"
        "*.ilk"
        "*.exp"
        "*.lib"
    )

    # Include CPack configuration
    list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
    include(CPack)
endif()

# Print configuration summary
message(STATUS "")
message(STATUS "Configuration Summary:")
message(STATUS "  Project: ${PROJECT_NAME} ${PROJECT_VERSION}")
message(STATUS "  Platform: ${PLATFORM_NAME}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Shared libraries: ${BUILD_SHARED_LIBS}")
message(STATUS "  Tests enabled: ${ENABLE_TESTS}")
message(STATUS "  Packaging enabled: ${ENABLE_PACKAGING}")
message(STATUS "  SSL support: ${ENABLE_SSL}")
message(STATUS "  JSON support: ${ENABLE_JSON}")
message(STATUS "")
