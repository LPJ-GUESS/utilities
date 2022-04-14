# A script to merge eg. LPJ-GUESS output with observations.
# First argument is the file that will be used to create the index, in the script below, only lon and lat will be used to create the index. 
# Add $3 like this: $1$2$3, if tou want to include year and $4 if doy is needed.
# The script is sensitive to the resolution of the data, eg. 64.250 will not be matched with 64.25. 
# In such a case, modify: a[$1$2], to: a[$1"0"$2"0"].
# Second argument is the file that you like to select data from, usually LPJ-GUESS output.
# The third argument is the name of the output file.
# In the script below, the fourth column in both files will be in the final output together with lon and lat (assuming that it's an LPJ-GUESS output file).
awk 'BEGIN {FS = " "};FNR==NR{a[$1$2]=$4;next}BEGIN{OFS=" "};{if($1$2 in a){print $1,$2,$4,a[$1$2]}}' $1 $2 > $3
