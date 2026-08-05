#!/bin/bash
# Always run this script from the root project directory.

####################
# Helper Functions #
####################

# Function to print an error and exit
error_exit() {
    echo "Error: $1" 1>&2
    exit 1
}

mkdir_if_not_exist() {
    local dir_name="$1"
    if [ -d "$dir_name" ]; then
        echo "Directory '$dir_name' already exists."
    else
        mkdir "$dir_name" || error_exit "Failed to create directory '$dir_name'."
        echo "Directory '$dir_name' created successfully."
    fi
}

# Function to check if a command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Function to print available presets
print_presets() {
    if [ -f "CMakePresets.json" ]; then
        if command_exists "jq"; then
            echo "📋 Available CMake Presets (parsed directly from CMakePresets.json):"
            echo "  -- Configure Presets --"
            jq -r '.configurePresets[] | select(.hidden != true) | "    cmake --preset \((.name + "                  ")[0:18]) \(.displayName // "")"' CMakePresets.json
            
            echo ""
            echo "  -- Build Presets --"
            jq -r '.buildPresets[] | select(.hidden != true) | "    cmake --build --preset \(.name)"' CMakePresets.json
            
            echo ""
            echo "  -- Test Presets --"
            jq -r '.testPresets[] | select(.hidden != true) | "    ctest --preset \(.name)"' CMakePresets.json
            echo ""
        else
            echo "  [jq not found. Cannot parse CMakePresets.json for presets list.]"
            echo "  [Run '$0 --install-deps' to install dependencies including jq.]"
        fi
    else
        echo "  [Failed to print presets: CMakePresets.json not found in the current directory.]"
    fi
}

# Function to print usage
print_usage() {
    echo "Usage: $0 [OPTIONS]"
    echo ""
    echo "Options:"
    echo "  --install-deps        Install required OS dependencies (requires sudo/root privileges for package manager)."
    echo "  --build <preset>      Configure and build the specified CMake preset (assumes build preset is <preset>-build)."
    echo "  -h, --help            Show this help message and list available CMake presets."
    echo ""
    print_presets
}

# Function to install dependencies on Debian/Ubuntu
install_debian() {
    sudo apt update && sudo apt install -y cmake zlib1g zlib1g-dev jq ninja-build clang clang-tidy libomp-dev openmpi-bin libopenmpi-dev || return 1
    sudo apt install -y ccache 2>/dev/null || echo "Note: ccache could not be installed. Continuing without it..."
    sudo apt install -y iwyu 2>/dev/null || echo "Note: iwyu could not be installed. Continuing without it..."
    sudo apt install -y lcov 2>/dev/null || echo "Note: lcov could not be installed. Continuing without it..."
    return 0
}

# Function to install dependencies on RHEL/CentOS
install_rhel() {
    sudo dnf update && sudo dnf install -y cmake zlib zlib-devel jq ninja-build clang clang-tools-extra libomp-devel openmpi openmpi-devel || return 1
    sudo dnf install -y ccache 2>/dev/null || echo "Note: ccache could not be installed. Continuing without it..."
    sudo dnf install -y include-what-you-use 2>/dev/null || echo "Note: include-what-you-use could not be installed. Continuing without it..."
    sudo dnf install https://dl.fedoraproject.org/pub/epel/epel-release-latest-9.noarch.rpm && sudo dnf install lcov || echo "Note: lcov could not be installed. Continuing without it..."
    return 0
}

# Function to install dependencies on MacOS
install_macos() {
    xcode-select --install 2>/dev/null # Suppress error if already installed
    brew install cmake zlib llvm libomp jq ninja open-mpi doxygen dot || return 1
    brew install ccache 2>/dev/null || echo "Note: ccache could not be installed. Continuing without it..."
    brew install include-what-you-use 2>/dev/null || echo "Note: include-what-you-use could not be installed. Continuing without it..."
    brew install lcov 2>/dev/null || echo "Note: lcov could not be installed. Continuing without it..."
    return 0
}

#########################
# Setup Pre-Commit Hook #
#########################

# Currently having issues with Ubuntu. Disabled for now.
# cp scripts/hooks/pre-commit .git/hooks/pre-commit || error_exit "failed to copy pre-commit hook"
# chmod +x .git/hooks/pre-commit || error_exit "failed to set .git/hooks/pre-commit as executable"

###################
# Parse Arguments #
###################

# If no arguments are passed, print usage and exit
if [ $# -eq 0 ]; then
    print_usage
    exit 0
fi

INSTALL_DEPS=false
BUILD_PRESET=""

while [[ $# -gt 0 ]]; do
    case $1 in
        --install-deps)
            INSTALL_DEPS=true
            shift
            ;;
        --build)
            if [ -n "$2" ] && [[ "$2" != --* ]]; then
                BUILD_PRESET="$2"
                shift 2
            else
                echo "Error: Missing or invalid preset name for --build flag."
                echo ""
                print_usage
                exit 1
            fi
            ;;
        -h|--help)
            print_usage
            exit 0
            ;;
        *)
            echo "Error: Unknown option '$1'."
            echo ""
            print_usage
            exit 1
            ;;
    esac
done

########################
# Install Dependencies #
########################

if [ "$INSTALL_DEPS" = true ]; then
    # Check if running as root, abort if yes.
    if [ "$(id -u)" -eq 0 ]; then
        error_exit "Running the script as root may cause permission issues later. Please rerun the script as a normal user (sudo will be invoked internally)."
    fi

    OS="$(uname -s)"
    case "$OS" in
    Linux*)
        if [ -f /etc/debian_version ]; then
            install_debian || error_exit "Failed to install mandatory dependencies on Debian/Ubuntu."
        elif [ -f /etc/redhat-release ]; then
            install_rhel || error_exit "Failed to install mandatory dependencies on RHEL/CentOS."
        else
            error_exit "Unsupported Linux distribution."
        fi
        ;;
    Darwin*)
        install_macos || error_exit "Failed to install mandatory dependencies on MacOS."
        ;;
    *)
        error_exit "Unsupported operating system."
        ;;
    esac
    echo "✅ Dependencies installed successfully."
fi

############################
# Check Prerequisites      #
############################

# Only check for these if we are actually building
if [ -n "$BUILD_PRESET" ]; then
    command_not_found_msg="Try installing dependencies by running this script with --install-deps"
    for cmd in git cmake jq ninja; do
        command_exists "$cmd" || error_exit "command $cmd not found. $command_not_found_msg"
    done

    ############################
    # Build GamesmanOne Binary #
    ############################

    echo "========================================="
    echo "Configuring the '$BUILD_PRESET' preset..."
    echo "========================================="
    cmake --preset "$BUILD_PRESET" || error_exit "CMake failed to configure the '$BUILD_PRESET' preset."

    echo "========================================="
    echo "Building GamesmanOne ($BUILD_PRESET)..."
    echo "========================================="
    cmake --build --preset "${BUILD_PRESET}-build" -j || error_exit "CMake failed to build GamesmanOne."

    ############################
    # Post-Build Summary       #
    ############################

    echo ""
    echo "====================================================================="
    echo "✅ Build Complete ($BUILD_PRESET)!"
    echo "====================================================================="
    echo "The build has been successfully compiled."
    echo ""
    echo "🚀 Useful Commands:"
    echo "  Run the program:           ./bin/gamesman"
    echo "  Clean the build:           rm -rf build/$BUILD_PRESET"
    echo "  Wipe CMake cache:          rm build/$BUILD_PRESET/CMakeCache.txt"
fi

############################
# Final Preset Printout    #
############################

# If the script did work (install or build), show the presets at the very end
if [ "$INSTALL_DEPS" = true ] && [ -z "$BUILD_PRESET" ]; then
    echo ""
    echo "====================================================================="
    print_presets
fi
