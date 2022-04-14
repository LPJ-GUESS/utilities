# A script to select eg. LPJ-GUESS output based on input from a an auxiliary file.
# First argument is the file that will be used to create the index, useful for selecting data from a larger file by using a gridlist.
# The script is sensitive to the resolution of the data, eg. 64.250 will not be matched with 64.25. 
# In such a case, modify: a[$1$2], to: a[$1"0"$2"0"].
# Second argument is the file that you like to select data from, usually LPJ-GUESS output.
# The third argument is the name of the output file.
awk 'BEGIN {FS = " "};FNR==NR{a[$1$2]=$4;next}BEGIN{OFS=" "};{if($1$2 in a){print $0}}' $1 $2 > $3
