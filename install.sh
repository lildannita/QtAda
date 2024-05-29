#!/bin/bash

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
NC='\033[0m'

echo_info() {
    echo -e "${GREEN}$1${NC}"
}

echo_warning() {
    echo -e "${YELLOW}$1${NC}"
}

echo_error() {
    echo -e "${RED}$1${NC}"
}

set -e

echo_info "Current OS: $(bash ./tools/get_os_info.sh -f)"
OS_INFO=$(bash ./tools/get_os_info.sh -d)
OS_NAME=$(echo ${OS_INFO} | awk '{ print $1 }')
INSTALL_CMD=$(bash ./tools/get_packet_cmd.sh "${OS_NAME}")

if [ -n "$INSTALL_CMD" ]; then
    echo_info "Installing necessary packages for $OS_NAME..."
    eval $INSTALL_CMD
else
    echo_error "Failed to get install command for $OS_NAME."
    exit 1
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
cmake ..
make

# TODO: По идее предыдущих команд достаточно
# echo_info "Installing the application..."
# sudo make install

BIN_PATH=$(realpath $(pwd)/bin)
echo_info "Adding the directory $BIN_PATH to PATH"
echo "export PATH=\$PATH:$BIN_PATH" >> ~/.bashrc
echo "export PATH=\$PATH:$BIN_PATH" >> ~/.bash_profile
if [ -f ~/.zshrc ]; then
    echo "export PATH=\$PATH:$BIN_PATH" >> ~/.zshrc
fi

echo_info "Installation completed. Please run 'source ~/.bashrc' to update your PATH."
