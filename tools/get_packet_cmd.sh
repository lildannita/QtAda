#!/bin/bash

case ${1} in
    Manjaro|Arch)
        INSTALL_CMD="sudo pacman -S --needed qt5-base qt5-quickcontrols qt5-quickcontrols2 qt5-remoteobjects qt5-declarative cmake make gcc git python"
        ;;
    Debian|Ubuntu|Astra)
        INSTALL_CMD="sudo apt-get install -y qt5-default qtbase5-private-dev qt5-qmltooling-plugins libqt5remoteobjects5 qtdeclarative5-dev cmake make gcc git python3"
        ;;
    # CentOS|Oracle)
    #     INSTALL_CMD="sudo yum install -y qt5-qtbase qt5-qml qt5-qtdeclarative cmake make gcc git python3"
    #     ;;
    *)
        echo "Unsupported OS: $OS_DISTR_NAME"
        exit 1
        ;;
esac

echo "$INSTALL_CMD"
