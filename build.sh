#!/bin/bash

# Build script for DungeonTemplateLibrary-Godot (GDExtension)
# This script builds the DTL extension using SCons with precompiled godot-cpp

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# --- Configuration ---
PLATFORM=""
ARCH="x86_64"
SCONS_FLAGS=""
PROJECT_NAME="dtl"

# --- Helper Functions ---
show_usage() {
    echo -e "${YELLOW}Usage: $0 [linux|macos|windows] [options]${NC}"
    echo "  linux: Build for Linux (x86_64)"
    echo "  macos: Build for macOS (universal)"
    echo "  windows: Build for Windows (x86_64, cross-compile)"
    echo ""
    echo "Options:"
    echo "  --debug: Build debug version (default: release)"
    exit 1
}

# --- Platform-specific Setup ---
setup_linux() {
    PLATFORM="linux"
    ARCH="x86_64"
    SCONS_FLAGS="platform=linux"
    echo -e "${BLUE}=== DTL Build Script (Linux) ===${NC}"
}

setup_macos() {
    PLATFORM="macos"
    ARCH="universal"
    SCONS_FLAGS="platform=macos arch=universal"
    echo -e "${BLUE}=== DTL Build Script (macOS) ===${NC}"
}

setup_windows() {
    PLATFORM="windows"
    ARCH="x86_64"
    SCONS_FLAGS="platform=windows use_mingw=yes"
    echo -e "${BLUE}=== DTL Build Script (Windows Cross-Compile) ===${NC}"
}

# --- Build Functions ---

# Function to check if godot-cpp cache is valid
check_godotcpp_cache() {
    echo -e "${YELLOW}Checking godot-cpp cache...${NC}"

    local lib_ext="a"

    local required_files=(
        "include/godot-cpp/bin/libgodot-cpp.${PLATFORM}.template_release.${ARCH}.${lib_ext}"
        "include/godot-cpp/bin/libgodot-cpp.${PLATFORM}.template_debug.${ARCH}.${lib_ext}"
        "include/godot-cpp/gen/include"
        "include/godot-cpp/gen/src"
    )

    for file in "${required_files[@]}"; do
        if [ ! -e "$file" ]; then
            echo -e "${RED}Cache miss: $file not found${NC}"
            return 1
        fi
    done

    if [ -f "include/godot-cpp/.sconsign.dblite" ]; then
        echo -e "${GREEN}SCons signature file found${NC}"
    fi

    echo -e "${GREEN}godot-cpp cache is valid!${NC}"
    return 0
}

# Function to download and set up prebuilt godot-cpp
build_godotcpp() {
    echo -e "${YELLOW}Downloading prebuilt godot-cpp (cache miss)...${NC}"

    local godot_version="4.5-stable"
    local download_url="https://github.com/NodotProject/godot-cpp-builds/releases/download/godot-${godot_version}/godot-cpp-prebuilt-godot-${godot_version}.zip"
    local cache_dir=".cache"
    local zip_file="${cache_dir}/godot-cpp-prebuilt-godot-${godot_version}.zip"

    mkdir -p "${cache_dir}"

    if [ ! -f "${zip_file}" ]; then
        echo -e "${BLUE}Downloading from: ${download_url}${NC}"
        if ! curl -L --fail "${download_url}" -o "${zip_file}"; then
            echo -e "${RED}Failed to download godot-cpp builds.${NC}"
            rm -f "${zip_file}"
            exit 1
        fi
    else
        echo -e "${GREEN}Found cached godot-cpp archive.${NC}"
    fi

    echo -e "${BLUE}Unzipping from ${zip_file}...${NC}"
    local temp_unzip_dir=$(mktemp -d)
    unzip -oq "${zip_file}" -d "${temp_unzip_dir}"

    if [ -d "${temp_unzip_dir}/godot-cpp-prebuilt" ]; then
        cp -r "${temp_unzip_dir}/godot-cpp-prebuilt/"* include/godot-cpp/
    else
        cp -r "${temp_unzip_dir}/"* include/godot-cpp/
    fi

    rm -rf "${temp_unzip_dir}"

    echo -e "${GREEN}godot-cpp setup completed!${NC}"
}

# Function to install dependencies
install_dependencies() {
    echo -e "${YELLOW}Checking dependencies for ${PLATFORM}...${NC}"

    local required_tools=("scons")

    if [ "$PLATFORM" == "linux" ]; then
        if [[ "$OSTYPE" != "linux-gnu"* ]]; then
            echo -e "${RED}Linux build requires a Linux environment. Current OS: $OSTYPE${NC}"
            exit 1
        fi
        required_tools+=("g++")
    elif [ "$PLATFORM" == "windows" ]; then
        required_tools+=("x86_64-w64-mingw32-gcc" "x86_64-w64-mingw32-g++")
    elif [ "$PLATFORM" == "macos" ]; then
        if [[ "$OSTYPE" != "darwin"* ]]; then
            echo -e "${RED}macOS build requires a macOS environment. Current OS: $OSTYPE${NC}"
            exit 1
        fi
        required_tools+=("clang++")
    fi

    local missing_tools=()
    for tool in "${required_tools[@]}"; do
        if ! command -v "$tool" &> /dev/null; then
            missing_tools+=("$tool")
        fi
    done

    if [ ${#missing_tools[@]} -ne 0 ]; then
        echo -e "${RED}Missing required tools: ${missing_tools[*]}${NC}"
        echo -e "${YELLOW}Please install them for your system.${NC}"
        exit 1
    fi

    echo -e "${GREEN}All required dependencies are available${NC}"
}

# Function to build the main project
build_main_project() {
    echo -e "${YELLOW}Building ${PROJECT_NAME}...${NC}"
    scons $SCONS_FLAGS
    echo -e "${GREEN}${PROJECT_NAME} build completed!${NC}"
}

# Function to show cache status
show_cache_status() {
    echo -e "${BLUE}=== Build Cache Status ===${NC}"

    local lib_ext="a"

    if [ -f "include/godot-cpp/bin/libgodot-cpp.${PLATFORM}.template_release.${ARCH}.${lib_ext}" ]; then
        echo -e "${GREEN}✓ Release library cached${NC}"
    else
        echo -e "${RED}✗ Release library missing${NC}"
    fi

    if [ -f "include/godot-cpp/bin/libgodot-cpp.${PLATFORM}.template_debug.${ARCH}.${lib_ext}" ]; then
        echo -e "${GREEN}✓ Debug library cached${NC}"
    else
        echo -e "${RED}✗ Debug library missing${NC}"
    fi

    if [ -d "include/godot-cpp/gen/include" ] && [ -d "include/godot-cpp/gen/src" ]; then
        echo -e "${GREEN}✓ Generated bindings cached${NC}"
    else
        echo -e "${RED}✗ Generated bindings missing${NC}"
    fi

    if [ -f "include/godot-cpp/.sconsign.dblite" ]; then
        echo -e "${GREEN}✓ SCons signature file present${NC}"
    else
        echo -e "${YELLOW}! SCons signature file missing (will be created)${NC}"
    fi

    echo ""
}

# --- Main Execution ---
main() {
    # Parse command-line arguments
    if [ -z "$1" ]; then
        show_usage
    fi

    local BUILD_TARGET="template_release"

    while [[ $# -gt 0 ]]; do
        case "$1" in
            linux)
                setup_linux
                shift
                ;;
            macos)
                setup_macos
                shift
                ;;
            windows)
                export CC=x86_64-w64-mingw32-gcc
                export CXX=x86_64-w64-mingw32-g++
                setup_windows
                shift
                ;;
            --debug)
                BUILD_TARGET="template_debug"
                shift
                ;;
            *)
                show_usage
                ;;
        esac
    done

    if [ -z "$PLATFORM" ]; then
        show_usage
    fi

    # Append target to SCONS_FLAGS
    SCONS_FLAGS="$SCONS_FLAGS target=$BUILD_TARGET"

    echo -e "${BLUE}Platform: ${PLATFORM}, Architecture: ${ARCH}, Target: ${BUILD_TARGET}${NC}"
    echo ""

    install_dependencies
    show_cache_status

    if ! check_godotcpp_cache; then
        build_godotcpp
    else
        echo -e "${GREEN}Using cached godot-cpp build${NC}"
    fi

    echo ""
    build_main_project
    echo ""

    echo -e "${GREEN}=== Build Complete! ===${NC}"
    echo -e "${BLUE}Built for: ${PLATFORM} (${ARCH})${NC}"
    echo -e "${BLUE}Build type: ${BUILD_TARGET}${NC}"

    show_cache_status
}

main "$@"
