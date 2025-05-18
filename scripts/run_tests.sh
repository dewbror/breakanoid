#!/bin/bash

dir=""

if [ -f "./build_debwithcov/DebWithCov/break_tests" ]; then
    tests="./build_debwithcov/DebWithCov/break_tests"
    dir="./build_debwithcov/"
elif [ -f "./build_debug/Debug/break_tests" ]; then
    tests="./build_debug/Debug/break_tests"
elif [ -f "./build_release/Release/break_tests" ]; then
    tests="./build_release/Release/break_tests"
else
    echo "Test build not found"
    return
fi

echo "Running ${tests}"
"${tests}" || exit 1
echo ""

if [ "${dir}" == "./build_debwithcov/" ]; then
    echo "Generating coverage info"
    lcov --capture --directory "${dir}" --output-file "${dir}/coverage.info" || exit 1
    lcov --extract "${dir}/coverage.info" '*/src/*' --output-file "${dir}/coverage.src.info" || exit 1
    genhtml "${dir}/coverage.src.info" --output-directory "${dir}/coverage_html" || exit 1
    rm "${dir}/coverage.info" || exit 1
fi
