#!/bin/bash
echo "Running unit tests..."

# Example: Check if streamer runs without crashing
./altitude_streamer 127.0.0.1 5001 3 test_out.csv 1
if [ -f test_out.csv ]; then
    echo "Streamer test passed"
else
    echo "Streamer test failed"
    exit 1
fi

echo "All unit tests passed!"

