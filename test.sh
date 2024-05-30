#!/bin/bash

if [ -z "$1" ]; then
  LOG_FILE="/tmp/test_script_$(date '+%d%m%Y_%H%M%S').log"
else
  LOG_FILE="$1"
fi

RED='\033[1;31m'
GREEN='\033[1;32m'
BLUE='\033[1;34m'
NC='\033[0m'

absolute_path() {
    echo "$(cd "$(dirname "$1")"; pwd)/$(basename "$1")"
}

log_file() {
    local message=$1
    echo "$(date '+%Y-%m-%d %H:%M:%S') - $message" >> $LOG_FILE
}

log() {
    local message=$1
    echo -e "$(date '+%Y-%m-%d %H:%M:%S') - $message" | tee -a $LOG_FILE
}

log_info() {
    local message=$1
    log_file "$message"
    echo -e "$(date '+%Y-%m-%d %H:%M:%S') - ${BLUE}$message${NC}"
}

log_success() {
    local message=$1
    log_file "$message"
    echo -e "$(date '+%Y-%m-%d %H:%M:%S') - ${GREEN}$message${NC}"
}

log_error() {
    local message=$1
    log_file "$message"
    echo -e "$(date '+%Y-%m-%d %H:%M:%S') - ${RED}$message${NC}"
}

run_and_check() {
    local command=$1
    local expected_result=$2
    local description=$3

    log_info "[Starting]: $description"
    log "Command: $command"

    START_ACTION_TIME=$(date +%s%3N)
    tmpfile=$(mktemp)
    eval $command > >(tee -a $LOG_FILE) 2> >(tee -a $LOG_FILE >&2) ; echo $? > $tmpfile
    result=$(< $tmpfile)
    rm -f $tmpfile
    END_ACTION_TIME=$(date +%s%3N)

    log_info "[Finished]: $description with result $result ($((END_ACTION_TIME - START_ACTION_TIME)) ms)"

    if [ $result -ne $expected_result ]; then
        log_error "Expected result $expected_result but got $result. Failing."
        exit 1
    else
        log_success "Expected result $expected_result achieved."
    fi
}

run_and_compare() {
    local command=$1
    local expected_file=$(absolute_path "$2")
    local generated_file=$(absolute_path "$3")
    local description=$4

    run_and_check "$command" 0 "$description"

    log "Comparing $generated_file with $expected_file"
    if ! python3 ./tools/compare_without_numbers.py "$generated_file" "$expected_file"; then
        log_error "Files $generated_file and $expected_file do not match. Failing."
        exit 1
    else
        log_success "Files $generated_file and $expected_file match."
    fi
}

main() {
    log_info "Starting test script"

    # Install
    run_and_check "./install.sh" 0 "Running install.sh"

    # Source bashrc
    run_and_check "source ~/.bashrc" 0 "Sourcing .bashrc"

    local QTADA="qtada"
    local RECORD_FLAG="-r"
    local UPDATE_FLAG="--update-script"
    local RUN_FLAG="-r"
    local AUTO_RECORD_FLAG="--auto-record"
    local AUTO_UPDATE_FLAG="--auto-update"

    local QTADA_EXAMPLES=$(absolute_path "./build/bin/examples")
    local QT_WIDGETS="$QTADA_EXAMPLES/simple_qt_widget_example"
    local QT_QUICK="$QTADA_EXAMPLES/simple_qt_quick_example"

    local TMP_DIR="/tmp"
    local TMP_WIDGETS_SCRIPT="$TMP_DIR/qt_widget.js"
    local TMP_QUICK_SCRIPT="$TMP_DIR/qt_quick.js"

    local REFERENCE_TESTS=$(absolute_path "./data")

    # Test generation for QtWidgets-based app
    run_and_compare "$QTADA $AUTO_RECORD_FLAG $RECORD_FLAG $TMP_WIDGETS_SCRIPT $QT_WIDGETS $AUTO_RECORD_FLAG" \
        "$REFERENCE_TESTS/auto_record_reference_for_qt_widgets.js" $TMP_WIDGETS_SCRIPT "Testing generation for QtWidgets-based app"

    # Test update for QtWidgets-based app
    run_and_compare "$QTADA $AUTO_RECORD_FLAG $UPDATE_FLAG 72 $RECORD_FLAG $TMP_WIDGETS_SCRIPT $QT_WIDGETS $AUTO_UPDATE_FLAG" \
        "$REFERENCE_TESTS/auto_update_reference_for_qt_widgets.js" $TMP_WIDGETS_SCRIPT "Testing update for QtWidgets-based app"

    # Test generation for QtQuick-based app
    run_and_compare "$QTADA $AUTO_RECORD_FLAG $RECORD_FLAG $TMP_QUICK_SCRIPT $QT_QUICK $AUTO_RECORD_FLAG" \
        "$REFERENCE_TESTS/auto_record_reference_for_qt_quick.js" $TMP_QUICK_SCRIPT "Testing generation for QtQuick-based app"

    # Test update for QtQuick-based app
    run_and_compare "$QTADA $AUTO_RECORD_FLAG $UPDATE_FLAG 52 $RECORD_FLAG $TMP_QUICK_SCRIPT $QT_QUICK $AUTO_UPDATE_FLAG" \
        "$REFERENCE_TESTS/auto_update_reference_for_qt_quick.js" $TMP_QUICK_SCRIPT "Testing update for QtQuick-based app"

    # Test script execution
    run_and_check "$QTADA -R $REFERENCE_TESTS/verify_reference_for_qt_widgets.js $QT_WIDGETS" 0 "Testing script execution for first app"
    run_and_check "$QTADA -R $REFERENCE_TESTS/verify_reference_for_qt_quick.js $QT_QUICK" 0 "Testing script execution for second app"

    # Test erroneous scripts
    run_and_check "$QTADA -R $REFERENCE_TESTS/error_js_syntax.js $QT_WIDGETS" 1 "Testing erroneous script with incorrect JS-syntax"
    run_and_check "$QTADA -R $REFERENCE_TESTS/error_qtada_syntax.js $QT_QUICK" 1 "Testing erroneous script with incorrect QtAda-syntax"

    # Test wrong arguments
    run_and_check "$QTADA --error-arg" 1 "Testing wrong arguments"

    # Code style check
    run_and_check "find . -iname '*.cpp' -o -iname '*.h*' | grep -v './build' | xargs clang-format --dry-run --Werror" 0 "Checking C++ code style"

    log_success "Test script completed successfully"
    log "Log file saved to $LOG_FILE"
    exit 0
}

main "$@"
