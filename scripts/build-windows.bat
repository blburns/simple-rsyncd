@echo off
REM Build script for Windows systems
REM Simple RSync Daemon - Build Script

setlocal enabledelayedexpansion

REM Set error handling
set "EXIT_CODE=0"

REM Colors for output (Windows 10+ supports ANSI colors)
set "RED=[91m"
set "GREEN=[92m"
set "YELLOW=[93m"
set "BLUE=[94m"
set "NC=[0m"

REM Function to print colored output
:print_status
echo %BLUE%[INFO]%NC% %~1
goto :eof

:print_success
echo %GREEN%[SUCCESS]%NC% %~1
goto :eof

:print_warning
echo %YELLOW%[WARNING]%NC% %~1
goto :eof

:print_error
echo %RED%[ERROR]%NC% %~1
goto :eof

REM Function to check if command exists
:command_exists
where "%~1" >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    set "COMMAND_EXISTS=true"
) else (
    set "COMMAND_EXISTS=false"
)
goto :eof

REM Function to detect Windows version
:detect_windows
call :print_status "Detecting Windows version..."
for /f "tokens=4-5 delims=. " %%i in ('ver') do set VERSION=%%i.%%j
call :print_status "Windows version: %VERSION%"

REM Check if running on Windows 10 or later
for /f "tokens=2 delims=." %%a in ('ver') do set MINOR=%%a
if %MINOR% LSS 10 (
    call :print_warning "Windows 10 or later recommended for best compatibility"
)
goto :eof

REM Function to check Visual Studio installation
:check_visual_studio
call :print_status "Checking Visual Studio installation..."

REM Check for Visual Studio 2017 or later
set "VS_FOUND=false"
set "VS_VERSION="

REM Check common Visual Studio installation paths
if exist "C:\Program Files (x86)\Microsoft Visual Studio\2017\*\MSBuild\Current\Bin\MSBuild.exe" (
    set "VS_FOUND=true"
    set "VS_VERSION=2017"
    set "VS_PATH=C:\Program Files (x86)\Microsoft Visual Studio\2017"
) else if exist "C:\Program Files\Microsoft Visual Studio\2019\*\MSBuild\Current\Bin\MSBuild.exe" (
    set "VS_FOUND=true"
    set "VS_VERSION=2019"
    set "VS_PATH=C:\Program Files\Microsoft Visual Studio\2019"
) else if exist "C:\Program Files\Microsoft Visual Studio\2022\*\MSBuild\Current\Bin\MSBuild.exe" (
    set "VS_FOUND=true"
    set "VS_VERSION=2022"
    set "VS_PATH=C:\Program Files\Microsoft Visual Studio\2022"
)

if "%VS_FOUND%"=="true" (
    call :print_success "Found Visual Studio %VS_VERSION%"
    set "VS_DEVENV=%VS_PATH%\*\Common7\IDE\devenv.exe"
    for %%i in ("%VS_DEVENV%") do set "VS_DEVENV=%%~fi"
) else (
    call :print_error "Visual Studio not found. Please install Visual Studio 2017 or later with C++ support."
    set "EXIT_CODE=1"
    goto :end
)
goto :eof

REM Function to check CMake installation
:check_cmake
call :print_status "Checking CMake installation..."
call :command_exists cmake
if "%COMMAND_EXISTS%"=="false" (
    call :print_error "CMake not found. Please install CMake from https://cmake.org/download/"
    set "EXIT_CODE=1"
    goto :end
) else (
    call :print_success "CMake found"
    cmake --version
)
goto :eof

REM Function to check vcpkg installation
:check_vcpkg
call :print_status "Checking vcpkg installation..."
if not exist "%USERPROFILE%\vcpkg\vcpkg.exe" (
    call :print_warning "vcpkg not found. Installing vcpkg..."
    call :install_vcpkg
) else (
    call :print_success "vcpkg found"
    "%USERPROFILE%\vcpkg\vcpkg.exe" version
)
goto :eof

REM Function to install vcpkg
:install_vcpkg
call :print_status "Installing vcpkg..."
cd /d "%USERPROFILE%"
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
call bootstrap-vcpkg.bat
call vcpkg integrate install
cd /d "%~dp0"
call :print_success "vcpkg installed successfully"
goto :eof

REM Function to install dependencies via vcpkg
:install_dependencies
call :print_status "Installing dependencies via vcpkg..."
"%USERPROFILE%\vcpkg\vcpkg.exe" install openssl jsoncpp
if %ERRORLEVEL% NEQ 0 (
    call :print_error "Failed to install dependencies"
    set "EXIT_CODE=1"
    goto :end
)
call :print_success "Dependencies installed successfully"
goto :eof

REM Function to build the project
:build_project
call :print_status "Building Simple RSync Daemon..."

REM Create build directory
if exist "build" (
    call :print_warning "Build directory already exists, cleaning..."
    rmdir /s /q build
)

mkdir build
cd build

REM Configure with CMake
call :print_status "Configuring with CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON -DBUILD_EXAMPLES=ON -DENABLE_LOGGING=ON -DENABLE_SSL=ON -DCMAKE_TOOLCHAIN_FILE="%USERPROFILE%\vcpkg\scripts\buildsystems\vcpkg.cmake"
if %ERRORLEVEL% NEQ 0 (
    call :print_error "CMake configuration failed"
    set "EXIT_CODE=1"
    goto :end
)

REM Build
call :print_status "Building project..."
cmake --build . --config Release
if %ERRORLEVEL% NEQ 0 (
    call :print_error "Build failed"
    set "EXIT_CODE=1"
    goto :end
)

call :print_success "Build completed successfully"
cd ..
goto :eof

REM Function to run tests
:run_tests
call :print_status "Running tests..."
cd build
if exist "Makefile" (
    make test
    if %ERRORLEVEL% NEQ 0 (
        call :print_warning "Some tests failed"
    ) else (
        call :print_success "Tests completed successfully"
    )
) else (
    call :print_warning "No tests found or build not completed"
)
cd ..
goto :eof

REM Function to install the project
:install_project
call :print_status "Installing Simple RSync Daemon..."
cd build
if exist "Makefile" (
    cmake --install .
    if %ERRORLEVEL% NEQ 0 (
        call :print_error "Installation failed"
        set "EXIT_CODE=1"
        goto :end
    )
    
    call :print_success "Installation completed successfully"
    
    REM Show installation info
    echo.
    call :print_status "Installation Summary:"
    echo   Binary: C:\Program Files\simple-rsyncd\bin\simple-rsyncd.exe
    echo   Library: C:\Program Files\simple-rsyncd\lib\simple-rsyncd.lib
    echo   Headers: C:\Program Files\simple-rsyncd\include\simple-rsyncd\
    echo   Config: C:\Program Files\simple-rsyncd\etc\simple-rsyncd\
    echo   Docs: C:\Program Files\simple-rsyncd\share\simple-rsyncd\docs\
    
    REM Test binary
    if exist "C:\Program Files\simple-rsyncd\bin\simple-rsyncd.exe" (
        echo.
        call :print_status "Testing binary..."
        "C:\Program Files\simple-rsyncd\bin\simple-rsyncd.exe" --version
    )
) else (
    call :print_error "Build not completed, cannot install"
    set "EXIT_CODE=1"
    goto :end
)
cd ..
goto :eof

REM Function to create package
:create_package
call :print_status "Creating package..."
cd build
if exist "Makefile" (
    REM Check if cpack is available
    call :command_exists cpack
    if "%COMMAND_EXISTS%"=="true" (
        cmake --build . --target package
        if %ERRORLEVEL% NEQ 0 (
            call :print_warning "Package creation failed"
        ) else (
            call :print_success "Package created successfully"
            
            REM List created packages
            dir *.msi *.zip 2>nul
            if %ERRORLEVEL% NEQ 0 (
                call :print_warning "No packages found"
            )
        )
    ) else (
        call :print_warning "cpack not available, skipping package creation"
    )
) else (
    call :print_error "Build not completed, cannot create package"
    set "EXIT_CODE=1"
    goto :end
)
cd ..
goto :eof

REM Function to create Windows service
:create_windows_service
call :print_status "Creating Windows service..."
if exist "C:\Program Files\simple-rsyncd\bin\simple-rsyncd.exe" (
    REM Check if sc.exe is available
    call :command_exists sc
    if "%COMMAND_EXISTS%"=="true" (
        sc create "SimpleRSyncDaemon" binPath= "C:\Program Files\simple-rsyncd\bin\simple-rsyncd.exe start --daemon" start= auto
        if %ERRORLEVEL% EQU 0 (
            call :print_success "Windows service created successfully"
            echo.
            call :print_status "Service management commands:"
            echo   sc start SimpleRSyncDaemon
            echo   sc query SimpleRSyncDaemon
            echo   sc stop SimpleRSyncDaemon
            echo   sc delete SimpleRSyncDaemon
        ) else (
            call :print_warning "Failed to create Windows service"
        )
    ) else (
        call :print_warning "sc.exe not available, cannot create service"
    )
) else (
    call :print_warning "Binary not found, cannot create service"
)
goto :eof

REM Function to show help
:show_help
echo Usage: %~nx0 [OPTIONS]
echo.
echo Options:
echo   -h, --help          Show this help message
echo   -d, --deps          Install dependencies only
echo   -b, --build         Build project only
echo   -t, --test          Run tests only
echo   -i, --install       Install project only
echo   -p, --package       Create package only
echo   -s, --service       Create Windows service
echo   -a, --all           Full build and install (default)
echo.
echo Examples:
echo   %~nx0               # Full build and install
echo   %~nx0 --deps        # Install dependencies only
echo   %~nx0 --build       # Build project only
echo   %~nx0 --install     # Install from existing build
echo   %~nx0 --service     # Create Windows service
goto :eof

REM Main function
:main
call :print_status "Simple RSync Daemon Build Script for Windows"
echo.

REM Parse command line arguments
set "INSTALL_DEPS=false"
set "BUILD_PROJECT=false"
set "RUN_TESTS=false"
set "INSTALL_PROJECT=false"
set "CREATE_PACKAGE=false"
set "CREATE_SERVICE=false"
set "FULL_BUILD=true"

:parse_args
if "%~1"=="" goto :end_parse
if "%~1"=="-h" goto :help
if "%~1"=="--help" goto :help
if "%~1"=="-d" goto :set_deps
if "%~1"=="--deps" goto :set_deps
if "%~1"=="-b" goto :set_build
if "%~1"=="--build" goto :set_build
if "%~1"=="-t" goto :set_test
if "%~1"=="--test" goto :set_test
if "%~1"=="-i" goto :set_install
if "%~1"=="--install" goto :set_install
if "%~1"=="-p" goto :set_package
if "%~1"=="--package" goto :set_package
if "%~1"=="-s" goto :set_service
if "%~1"=="--service" goto :set_service
if "%~1"=="-a" goto :set_all
if "%~1"=="--all" goto :set_all
call :print_error "Unknown option: %~1"
call :show_help
set "EXIT_CODE=1"
goto :end

:help
call :show_help
goto :end

:set_deps
set "INSTALL_DEPS=true"
set "FULL_BUILD=false"
shift
goto :parse_args

:set_build
set "BUILD_PROJECT=true"
set "FULL_BUILD=false"
shift
goto :parse_args

:set_test
set "RUN_TESTS=true"
set "FULL_BUILD=false"
shift
goto :parse_args

:set_install
set "INSTALL_PROJECT=true"
set "FULL_BUILD=false"
shift
goto :parse_args

:set_package
set "CREATE_PACKAGE=true"
set "FULL_BUILD=false"
shift
goto :parse_args

:set_service
set "CREATE_SERVICE=true"
set "FULL_BUILD=false"
shift
goto :parse_args

:set_all
set "FULL_BUILD=true"
shift
goto :parse_args

:end_parse

REM Check if running as administrator
net session >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    call :print_warning "Not running as administrator. Some operations may fail."
)

REM Check if we're in the right directory
if not exist "CMakeLists.txt" (
    call :print_error "CMakeLists.txt not found. Please run this script from the project root directory."
    set "EXIT_CODE=1"
    goto :end
)

REM Detect Windows version
call :detect_windows

REM Check Visual Studio
call :check_visual_studio
if %EXIT_CODE% NEQ 0 goto :end

REM Check CMake
call :check_cmake
if %EXIT_CODE% NEQ 0 goto :end

REM Check vcpkg
call :check_vcpkg

REM Execute requested actions
if "%FULL_BUILD%"=="true" (
    call :print_status "Performing full build and install..."
    call :install_dependencies
    if %EXIT_CODE% NEQ 0 goto :end
    call :build_project
    if %EXIT_CODE% NEQ 0 goto :end
    call :run_tests
    call :install_project
    if %EXIT_CODE% NEQ 0 goto :end
    call :create_package
    call :create_windows_service
) else (
    if "%INSTALL_DEPS%"=="true" (
        call :install_dependencies
        if %EXIT_CODE% NEQ 0 goto :end
    )
    
    if "%BUILD_PROJECT%"=="true" (
        call :install_dependencies
        if %EXIT_CODE% NEQ 0 goto :end
        call :build_project
        if %EXIT_CODE% NEQ 0 goto :end
    )
    
    if "%RUN_TESTS%"=="true" (
        call :run_tests
    )
    
    if "%INSTALL_PROJECT%"=="true" (
        call :install_project
        if %EXIT_CODE% NEQ 0 goto :end
    )
    
    if "%CREATE_PACKAGE%"=="true" (
        call :create_package
    )
    
    if "%CREATE_SERVICE%"=="true" (
        call :create_windows_service
    )
)

call :print_success "Build script completed successfully!"
goto :end

:end
if %EXIT_CODE% NEQ 0 (
    call :print_error "Build script failed with exit code %EXIT_CODE%"
)
exit /b %EXIT_CODE%
