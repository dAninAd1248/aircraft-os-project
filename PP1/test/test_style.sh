#!/bin/bash

# Define the source file to check
SOURCE_FILE_GROUND="../ground_control/src/main.c"
HEADER_FILE_GROUND="../ground_control/include/functions.h"
FUNCTIONS_FILE_GROUND="../ground_control/src/functions.c"
SOURCE_FILEAIR_C="../air_control_c/src/main.c"
HEADER_FILEAIR_C="../air_control_c/include/functions.h"
FUNCTIONS_FILEAIR_C="../air_control_c/src/functions.c"

echo "--- Running static analysis with cpplint ---"
./cpplint.py --root=.. --filter=-whitespace/ending_newline,-runtime/threadsafe_fn,-build/include_subdir,-readability/copyright,-legal/copyright ${SOURCE_FILE_GROUND} ${HEADER_FILE_GROUND} ${FUNCTIONS_FILE_GROUND} ${SOURCE_FILEAIR_C} ${HEADER_FILEAIR_C} ${FUNCTIONS_FILEAIR_C}> cpplint_report.txt 2>&1

# Sum the severity values (numbers in square brackets at the end of each error line)
SEVERITY_SUM=$(grep -o '\[[0-9]\+\]$' cpplint_report.txt | grep -o '[0-9]\+' | awk '{s+=$1} END {print s+0}')

# Calculate the style score (100 minus 5 points per severity, minimum 0)
STYLE_SCORE=$(( 100 - SEVERITY_SUM * 2))
if [ $STYLE_SCORE -lt 0 ]; then
    STYLE_SCORE=0
fi

echo "--- Style Check Report ---"
cat cpplint_report.txt
echo "--------------------------"
echo "Style Score: $STYLE_SCORE out of 100"

# Clean up
#rm cpplint_report.txt