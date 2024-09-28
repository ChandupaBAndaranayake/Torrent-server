#!/bin/bash

SERVER_DIR="./server_folder"
CLIENT_DIR="./client_folder"
server="./Server"
client="./Client"

mkdir -p $SERVER_DIR
mkdir -p $client
mkdir -p $server
mkdir -p $CLIENT_DIR

NUM_FILES=50

FILE_SIZE=$((10 * 1024 * 1024))  # 10MB in bytes

echo "Generating $NUM_FILES files in $SERVER_DIR..."
for i in $(seq 1 $NUM_FILES); do
    dd if=/dev/urandom of=$SERVER_DIR/file_$i.bin bs=$FILE_SIZE count=1 status=none
    echo "Created $SERVER_DIR/file_$i.bin (10MB)"
done

echo "Generating $NUM_FILES files in $CLIENT_DIR..."
for i in $(seq 1 $NUM_FILES); do
    dd if=/dev/urandom of=$CLIENT_DIR/file_$i.bin bs=$FILE_SIZE count=1 status=none
    echo "Created $CLIENT_DIR/file_$i.bin (10MB)"
done

echo "File generation complete."
