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

# Function to install dependencies on Debian/Ubuntu
install_debian() {
    # Required packages
    sudo apt update && sudo apt install -y cmake zlib1g zlib1g-dev jq ninja-build clang clang-tidy libomp-dev openmpi-bin libopenmpi-dev || return 1
    
    # Attempt the following packages separately so failure doesn't abort the setup
    sudo apt install -y ccache 2>/dev/null || echo "Note: ccache could not be installed. Continuing without it..."
    sudo apt install -y iwyu 2>/dev/null || echo "Note: iwyu could not be installed. Continuing without it..."
    sudo apt install -y lcov 2>/dev/null || echo "Note: lcov could not be installed. Continuing without it..."
    return 0
}

# Function to install dependencies on RHEL/CentOS
install_rhel() {
    # Required packages
    sudo dnf update && sudo dnf install -y cmake zlib zlib-devel jq ninja-build clang clang-tools-extra libomp-devel openmpi openmpi-devel || return 1
    
    sudo dnf install -y ccache 2>/dev/null || echo "Note: ccache could not be installed. Continuing without it..."
    sudo dnf install -y include-what-you-use 2>/dev/null || echo "Note: include-what-you-use could not be installed. Continuing without it..."
    sudo dnf install https://dl.fedoraproject.org/pub/epel/epel-release-latest-9.noarch.rpm && sudo dnf install lcov || echo "Note: lcov could not be installed. Continuing without it..."
    return 0
}

# Function to install dependencies on MacOS
install_macos() {
    # Assuming Homebrew is installed
    xcode-select --install 2>/dev/null # Suppress error if already installed
    
    # Added: jq, ninja, open-mpi (llvm provides clang/clang-tidy)
    brew install cmake zlib llvm libomp jq ninja open-mpi || return 1
    
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

########################
# Install Dependencies #
########################

# Check if running as root, abort if yes.
if [ "$(id -u)" -eq 0 ]; then
    error_exit "Running the script as root may cause permission issues later. Please rerun the script as normal user."
fi

# Detect OS and architecture
OS="$(uname -s)"
ARCH="$(uname -m)"

# Parse command-line arguments
INSTALL_DEPENDENCIES=false

for arg in "$@"; do
    case $arg in
        --install)
            INSTALL_DEPENDENCIES=true
            ;;
    esac
done

# Install dependencies if requested.
if [ "$INSTALL_DEPENDENCIES" = true ]; then
    case "$OS" in
    Linux*)
        # Detect if using Debian/Ubuntu or RHEL/CentOS
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
else
    echo "Skipping installation of dependencies."
fi

# Check for required commands
command_not_found_msg="Try installing dependencies by running this script with --install"
for cmd in git cmake jq ninja; do
    command_exists "$cmd" || error_exit "command $cmd not found. $command_not_found_msg"
done

############################
# Build GamesmanOne Binary #
############################

echo "========================================="
echo "Configuring the 'release' preset..."
echo "========================================="
cmake --preset release || error_exit "CMake failed to configure the release preset."

echo "========================================="
echo "Building GamesmanOne (Release)..."
echo "========================================="
cmake --build --preset release-build -j || error_exit "CMake failed to build GamesmanOne."

############################
# Post-Build Summary       #
############################

echo ""
echo "====================================================================="
echo "✅ Setup and Build Complete!"
echo "====================================================================="
echo "The release build has been successfully compiled into the bin/ directory."
echo ""
echo "🚀 Useful Commands:"
echo "  Run the program:          ./bin/gamesman"
echo "  Clean the release build:  rm -rf build/release"
echo "  Wipe CMake cache:         rm build/release/CMakeCache.txt"
echo ""

# Use jq to parse the JSON and format the output, ignoring hidden base presets.
if command_exists "jq"; then
    echo "📋 Available CMake Presets (parsed directly from CMakePresets.json):"
    echo "  -- Configure Presets --"
    # Appends 18 spaces to the name, then slices the first 18 characters for perfect alignment
    jq -r '.configurePresets[] | select(.hidden != true) | "    cmake --preset \((.name + "                  ")[0:18]) \(.displayName // "")"' CMakePresets.json
    
    echo ""
    echo "  -- Build Presets --"
    jq -r '.buildPresets[] | select(.hidden != true) | "    cmake --build --preset \(.name)"' CMakePresets.json
    
    echo ""
    echo "  -- Test Presets --"
    jq -r '.testPresets[] | select(.hidden != true) | "    ctest --preset \(.name)"' CMakePresets.json
else
    echo "  [jq not found. Cannot parse CMakePresets.json for presets list.]"
fi
