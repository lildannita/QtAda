#!/bin/bash

REPO_DIR="$HOME/QtAda"
LOG_DIR="/logs"
LOG_PATH="${LOG_DIR}/test_script_$(date '+%d%m%Y_%H%M%S').log"
mkdir -p $LOG_DIR

git clone https://github.com/lildannita/QtAda.git $REPO_DIR

cd $REPO_DIR

./test.sh "${LOG_PATH}"
