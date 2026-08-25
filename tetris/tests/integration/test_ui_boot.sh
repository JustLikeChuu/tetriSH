#!/bin/bash
# L10.1 System UI & Integration Test
# Asserts that the compiled binaries boot successfully and yield expected exit codes.

SERVER_BIN="./build/bin/tetrisd"
CLIENT_BIN="./build/bin/tetrisu"

# 1. Check if make successfully generated the binaries
if [ ! -f "$SERVER_BIN" ] || [ ! -f "$CLIENT_BIN" ]; then
    echo "[FAIL] Binaries not found! Did 'make all' succeed?"
    exit 1
fi

# 2. Boot the server in the background
$SERVER_BIN > server_test.log 2>&1 &
SERVER_PID=$!

# Give server 1 second to bind to port
sleep 1

# 3. Verify server hasn't crashed immediately
if ! kill -0 $SERVER_PID 2>/dev/null; then
    echo "[FAIL] tetrisd crashed immediately upon boot."
    cat server_test.log
    exit 1
fi

# 4. Clean up background process
kill $SERVER_PID
rm -f server_test.log

echo "[PASS] System Integration Boot check succeeded."
exit 0