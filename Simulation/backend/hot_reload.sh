#!/bin/bash

# Initial build
mkdir -p build
cd build
cmake ..
make

# Run the application in the background
./crow_app &

# Watch for changes in the src directory
cd ..
while inotifywait -e modify,create,delete -r src/; do
    echo "Changes detected, rebuilding..."
    cd build
    make
    pkill crow_app
    ./crow_app &
    cd ..
done
