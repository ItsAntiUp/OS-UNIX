#!/bin/sh
#Program to list top browsers in an Apache log file
#Author: Kostas Ragauskas, INF, course 3, group 2

#An auxillary messages just in case the inputed arguments are incorrect

#Checking if argument count is > 0
if [ $# -eq 0 ]
   then
      echo "ERROR: No arguments!" >&2
      exit 1
fi

#Checking if argument count is < 2
if [ ! -z "$2" ]
   then
      echo "ERROR: Too many arguments! ($#)" >&2
      exit 1
fi

#Checking if argument is a number
[ "$1" -eq "$1" ] 2>/dev/null
if [ $? -ne 0 ]
   then
       echo "ERROR: $1 is not a number" >&2
       exit 1
fi

#Checking if argument is not negative
if [ "$1" -lt 1 ]
   then
      echo "ERROR: $1 is not a valid unsigned integer" >&2
      exit 1
fi

#Actual program
echo "\nTop $1 browsers:\n"

#Splitting each line by ", removing parentheses, splitting by /, 
#trimming, sorting and printing in descending order
cut -d '"' -f6 access.log | 
sed 's/[(][^)]*[)]/ /g' |
cut -d '/' -f3 |
tr -s ' ' |
cut -d ' ' -f2 |
grep -v '^$' |
grep -v 'Version' |
grep -v '[.]\|_\|-\|)\|(\|\n|\t' |
sed 's/20100101//g' |
tr -d ' ' |
sort |
uniq -c | 
sort -n -r |
sed 's/^ * //g' |
cut -d ' ' -f2 |
grep -vn '^$' |
head -$1
