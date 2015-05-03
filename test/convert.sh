#!/bin/bash

# Simple test file for svx2vdb. Converts a file and checks existence of the output
result=0

../svx2vdb ../examples/urchin.svx temp.vdb
if test $? -eq 0
	then
		echo "Execution returned success" $?
	else
		echo "Execution returned failure" $?
		result=1
fi

output="temp.vdb"
if [ -f $output ]
	then
		echo "Output file" $output "found"
		rm temp.vdb
	else
		echo "Output file" $output "not found"
		result=1
fi

exit $result

