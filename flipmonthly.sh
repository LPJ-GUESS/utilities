# A script to flip the monthly columns from LPJ-GUESS output to one column, like the ordinary annual output.
x=$1
c=${x%%.*}
awk -v c="$c" '{if(NR==1){print "lon","lat","year","month", c} else { for(i=4;i<=NF;i++){j=i-3;print $1,$2,$3,j,$i}}}' $1 > $c"_col.dat"
