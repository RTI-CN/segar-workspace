#!/bin/bash

if [ $# -lt 1 ]; then
    echo "usage: clang-format.sh {folder1} [folder2 ... folderN]"
    exit 1
fi

# Define folders to exclude. Use absolute paths for precise matching.
exclude_folders=("")
# Function to find source/header files
find_files() {
    local folder=$1
    local find_command="find \"$folder\""

    # Build exclusion predicates with absolute paths
    for exclude in "${exclude_folders[@]}"; do
        find_command+=" -path \"$exclude\" -prune -o"
    done

    # Finalize find command, excluding generated protobuf files
    find_command+=" \( -name \"*.h\" -o -name \"*.hpp\" -o -name \"*.cpp\" -o -name \"*.cc\" \) ! -name \"*.pb.h\" ! -name \"*.pb.hpp\" ! -name \"*.pb.cc\" -print"

    # Execute find command
    eval "$find_command"
}

# Iterate through input folders
for folder in "$@"; do
    files=$(find_files "$folder")

    for file in $files; do
        echo "$file"
        clang-format --style=file -i "$file"
    done
done
