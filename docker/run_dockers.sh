#!/bin/bash

DISTROS=("ubuntu")
LOGS_DIR="logs"

for DISTRO in "${DISTROS[@]}"; do
    docker build -t ${DISTRO}_test -f docker/Dockerfile.${DISTRO} .
    docker run --rm -v $(pwd)/$LOGS_DIR:/logs ${DISTRO}_test
done

RESULT_DIR="/tmp/results_$(date '+%d%m%Y_%H%M%S')"
mkdir -p $RESULT_DIR

for DISTRO in "${DISTROS[@]}"; do
    cp -r $LOGS_DIR/* $RESULT_DIR/${DISTRO}
done

rm -rf $LOGS_DIR
