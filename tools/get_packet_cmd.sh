#!/bin/bash

PACKAGES_ARCH="qt5-base qt5-quickcontrols qt5-quickcontrols2 qt5-remoteobjects qt5-declarative cmake make gcc git python"
PACKAGES_DEBIAN="qtbase5-dev qtdeclarative5-dev cmake make gcc git python3"
PACKAGES_UBUNTU="qt5-default qtbase5-private-dev qt5-qmltooling-plugins libqt5remoteobjects5 qtdeclarative5-dev cmake make gcc git python3"
PACKAGES_ASTRA="qt5-default qtbase5-dev qt5-qmltooling-plugins libqt5remoteobjects5 qtdeclarative5-dev cmake make gcc git python3"

# CentOS|Oracle)
#     INSTALL_CMD="sudo yum install -y qt5-qtbase qt5-qml qt5-qtdeclarative cmake make gcc git python3"
#     ;;


function show_help() {
    echo "Usage: $0 [options] <Linux Distribution Name>"
    echo ""
    echo "Options:"
    echo "  -h, --help                  print help"
    echo "  -i, --install-packages      generate command to check for packages"
    echo "  -c, --check-packages        generate command to install packages"
}

INSTALL_PACKAGES=true
OS=
while [[ $# -gt 0 ]]; do
    case "$1" in
        -c|--check-packages)
            INSTALL_PACKAGES=false
            shift
            ;;
        -i|--install-packages)
            INSTALL_PACKAGES=true
            shift
            ;;
        -h|--help)
            show_help
            exit 0
            ;;
        *)
            OS=$1
            shift
            ;;
    esac
done

if $INSTALL_PACKAGES; then
    case $OS in
        Manjaro|Arch)
            INSTALL_CMD="sudo pacman -S --needed $PACKAGES_ARCH"
            ;;
        Debian)
            INSTALL_CMD="sudo apt-get install -y $PACKAGES_DEBIAN"
            ;;
        Ubuntu)
            INSTALL_CMD="sudo apt-get install -y $PACKAGES_UBUNTU"
            ;;
        Astra)
            INSTALL_CMD="sudo apt-get install -y $PACKAGES_ASTRA"
            ;;
        *)
            echo "Unsupported OS: $OS"
            exit 1
            ;;
    esac
    echo "$INSTALL_CMD"
else
    case $OS in
        Manjaro|Arch)
            CHECK_CMD="pacman -Q $PACKAGES_ARCH"
            ;;
        Debian)
            CHECK_CMD="dpkg -l $PACKAGES_DEBIAN"
            ;;
        Ubuntu)
            CHECK_CMD="dpkg -l $PACKAGES_UBUNTU"
            ;;
        Astra)
            CHECK_CMD="dpkg -l $PACKAGES_ASTRA"
            ;;
        *)
            echo "Unsupported OS: $OS"
            exit 1
            ;;
    esac
    echo "$CHECK_CMD"
fi
