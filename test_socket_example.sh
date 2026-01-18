#!/bin/bash

# Test script for SocketExample
# This script runs the SocketExample executable and captures output

set -e

echo "Building SocketExample..."
make clean
make .dist/SocketExample

echo ""
echo "Running SocketExample..."
echo "================================"

# Run the executable and capture output
OUTPUT=$(./.dist/SocketExample 2>&1)

echo "$OUTPUT"

echo ""
echo "================================"
echo "Test completed."
echo ""
echo "Output saved to: socket_example.log"
if [ -f "socket_example.log" ]; then
  echo ""
  echo "Log file contents:"
  cat socket_example.log
fi
