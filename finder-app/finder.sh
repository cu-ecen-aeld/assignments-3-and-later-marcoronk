#!/bin/bash

# Check if the correct number of arguments are provided
if [ $# -ne 2 ]; then
    echo "Error: Two arguments are required. Usage: ./finder.sh <directory> <search_string>"
    exit 1
fi

# Assign arguments to variables
filesdir="$1"
searchstr="$2"

# Check if the first argument is a valid directory
if [ ! -d "$filesdir" ]; then
    echo "Error: $filesdir is not a valid directory."
    exit 1
fi

# Initialize counters
file_count=0
matching_line_count=0

# Find all files in the directory and subdirectories
# Loop through each file in the directory recursively
while IFS= read -r -d '' file; do
    ((file_count++))  # Increment the file counter

    # Count matching lines in the current file
    matching_lines=$(grep -c -- "$searchstr" "$file")
    matching_line_count=$((matching_line_count + matching_lines))  # Add to the total match count
done < <(find "$filesdir" -type f -print0)  # Using find to get all files (-type f), -print0 handles filenames with spaces

# Output the results
echo "The number of files are $file_count and the number of matching lines are $matching_line_count"
