#!/bin/bash

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
NC='\033[0m'

echo_info() {
    echo -e "${GREEN}$1${NC}" >&2
}

echo_warning() {
    echo -e "${YELLOW}$1${NC}" >&2
}

echo_error() {
    echo -e "${RED}$1${NC}" >&2
}

quite_eval() {
    eval $1 >&2
}

set -e

INSTALL_PACKAGES=true
INSTALL_SYMLINK=true
for arg in "$@"; do
    case $arg in
        --skip-package-install)
            INSTALL_PACKAGES=false
            shift
            ;;
        --skip-symlink)
            INSTALL_SYMLINK=false
            shift
            ;;
    esac
done

echo_info "Current OS: $(bash ./tools/get_os_info.sh -f)"
OS_INFO=$(bash ./tools/get_os_info.sh -d)
OS_NAME=$(echo ${OS_INFO} | awk '{ print $1 }')

if $INSTALL_PACKAGES; then
    INSTALL_CMD=$(bash ./tools/get_packet_cmd.sh "${OS_NAME}")
    if [ -n "$INSTALL_CMD" ]; then
        echo_info "Installing necessary packages for $OS_NAME..."
        quite_eval "$INSTALL_CMD"
    else
        echo_error "Failed to get install command for $OS_NAME."
        exit 1
    fi
else
    CHECK_CMD=$(bash ./tools/get_packet_cmd.sh --check-packages "${OS_NAME}")
    if [ -n "$CHECK_CMD" ]; then
        echo_info "Checking necessary packages for $OS_NAME..."
        if quite_eval "$CHECK_CMD"; then
            echo_info "All necessary packages are already installed."
        else
            echo_error "Some necessary packages are missing."
            exit 1
        fi
    else
        echo_error "Failed to get check command for $OS_NAME."
        exit 1
    fi
fi

# TODO: Нужно будет как-нибудь корректно проверять актуальность репозитория
# echo_info "Checking repository status..."
# git fetch --all
# STATUS=$(git status -uno)
# if echo "$STATUS" | grep -q 'Your branch is up to date'; then
#     echo_info "The repository is up to date."
# else
#     echo_warning "The repository is not synchronized with the remote."
#     echo_info "Updating the repository..."
#     git pull --ff-only
# fi

echo_info "Building the project..."
mkdir -p build
cd build
quite_eval "cmake .."
quite_eval "make"

# TODO: По идее предыдущих команд достаточно
# echo_info "Installing the application..."
# sudo make install

BIN_PATH=$(realpath $(pwd)/bin)
APP_NAME=qtada

if $INSTALL_SYMLINK; then
    LINK_PATH=/usr/local/bin/$APP_NAME
    if [ -L $LINK_PATH ]; then
        sudo rm $LINK_PATH
    fi
    quite_eval "sudo ln -s $BIN_PATH/$APP_NAME /usr/local/bin/$APP_NAME"
    echo_info "Symbolic link created at '$LINK_PATH'. Call 'qtada' to run application"
    echo_info "Installation completed."
else
    echo "$BIN_PATH/$APP_NAME"
fi