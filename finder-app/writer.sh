#!/bin/bash

# Check if the correct number of arguments are provided
if [ $# -ne 2 ]; then
    echo "Error: Two arguments are required. Usage: ./writer.sh <file_path> <text_string>"
    exit 1
fi

# Assign arguments to variables
writefile="$1"
writestr="$2"

# Create the directory if it does not exist
dir=$(dirname "$writefile")
if [ ! -d "$dir" ]; then
    echo "Error: Directory $dir does not exist. Creating it now."
    mkdir -p "$dir"
    if [ $? -ne 0 ]; then
        echo "Error: Failed to create directory $dir."
        exit 1
    fi
fi

# Write the text string to the file, overwriting any existing content
echo "$writestr" > "$writefile"
if [ $? -ne 0 ]; then
    echo "Error: Failed to create or write to the file $writefile."
    exit 1
fi

echo "File $writefile has been created/overwritten successfully."
