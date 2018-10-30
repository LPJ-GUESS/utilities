#!/bin/bash 

function print_help {
    echo "  usage: lpjg_fast_analysis.sh <PATH_TO_RUNDIR> [-o <OUTPATH>] [-r <REFPATH>] [fwah]"
    echo ""
    echo "    PATH_TO_RUNDIR: should be to the level <rundir>/<exp-name>/output/lpjg" 
    echo "   Options:"
    echo "    -o --outdir          <PATH> to store output in, DEFAULT='./'"      
    echo "    -r --reference       <PATH> to a dir with aggregated LPJG reference run"
    echo "    -f --force_aggregate -> do exactly that, in case a previous aggregation has failed..."
    echo "    -w --overwrite       if outdir exists force overwriting of analysis data"
    echo "    -a --aggregate_all   aggregate the full set of LPJG output files"
    echo "    -h --help            print this again :)"
    echo ""
    exit 666
}

function aggregate_lpjg {
    inpath=$1
    outpath=$2
    full=$3

    # find first leg
    for ((x=1; x<999; x++))
    do 
	cf=${inpath}/$(printf "%3.3d" $x)
	if [ -d $cf ]
	then
	    cfinp=$cf
	    f=$x
	    break
	fi
    done

    # list of different out-files to process
    if [[ $full == 1 ]]
    then
	flist=$(find $cfinp -name '*.out' | sed "s%$cfinp\/%%g")
	#Create subdirectories
	if [ ! -d ${outpath}/CMIP6 ]; then mkdir -d ${outpath}/CMIP6; fi 
	if [ ! -d -d ${outpath}/CRESCENDO ]; then mkdir -d ${outpath}/CRESCENDO; fi
    else
	flist="cpool.out cflux.out nsources.out fpc.out lai.out seasonality.out"
    fi
    
    for infile in $flist
    do 
	echo "Processing $infile ..." 
	for ((x=f; x<999; x++))
	do
	    cf=${inpath}/$(printf "%3.3d" $x)
	    # check for existance of leg
	    if [ ! -d $cf ]
	    then
		break
	    fi
	    # earliest leg needs to copy header, too
	    if [ $x -eq $f ]
	    then
		cat $cf/$infile > ${outpath}/$infile
		#skip skip header line in subsequent files
	    else
		awk '(FNR>1){print $0}' $cf/$infile >> ${outpath}/$infile
	    fi
	done
    done
}

echo ""
echo "### LPJG - KWIK-CHECK ###  (-h for instructions)"
echo ""

# get args 
if [[ $# -lt 1 ]]
then 
    print_help
    exit 1
fi 

# input dir
if [ -d $1 ]
then
    new_run=$1
else
    echo " Directory $1 doesn't exist"
    exit 99
fi
shift 1

outputdir="./"
force_ow=0
force_agg=0
full=0
old_run="_none_"
run="run"
ref="ref"
while test $# -gt 0; do
    case "$1" in
        -o | --outdir)
            outputdir=$2
            shift 2
            ;;
        -r | --reference)
            old_run=$2
            shift 2
            ;;
        -f | --force_aggregate)
            force_agg=1
            force_ow=1
            shift 1
            ;;
        -w | --overwrite)
            force_ow=1
            shift 1
            ;;
        -a | --aggregate_all )
            full=1
            force_agg=1
            force_ow=1
            shift 1;
            ;;
        -h | --help )
            print_help
            return -1;
            ;;
	*)
            echo "$1 is not a recognized flag!"
            exit 99;
            ;;
    esac
done  

echo " Analysing run: " $new_run
if [[ $old_run != "_none_" ]]
then
    echo " Reference Run: " $old_run
fi 

if [ ! -d $outputdir ]
then
    mkdir -p $outputdir/$run
    if [[ $old_run != "_none_" ]]
    then
        mkdir -p $outputdir/$ref
    fi
    if [ ! $? ] 
    then
	exit -1
    fi
    echo " Output dir   : " $outputdir/$run "created"
    if [[ $old_run != "_none_" ]]
    then
        echo " Output dir   : " $outputdir/$ref "created"
    fi
elif [[ $force_ow == 1 ]]
then
    echo " Output dir   : " $outputdir 
else
    echo " directory $outputdir exists"
    echo ' use --overwrite to do overwrite existing files!'
    exit 3
fi
echo""

if [ -f ${outputdir}/${run}/cpool.out -a  $force_agg == 0 ]
then
    echo "Aggregated files exist, skipping aggregation (use -f to force aggregation)"
else
    echo "Aggregating LPJ-GUESS output-files"; echo""
    aggregate_lpjg $new_run $outputdir/$run $full
    echo "Aggregation complete."; echo""
fi

if [[ $old_run == "_none_" ]]
then 
    echo "No reference run chosen. Only plotting current run."; echo""
fi

module load MATLAB/R2018a-nsc1

aslice $outputdir/$run/cflux.out -sum 'kg/m2->Pg' -o $outputdir/$run/cflux_Pg.txt
aslice $outputdir/$run/cpool.out -sum 'kg/m2->Pg' -o $outputdir/$run/cpool_Pg.txt
aslice $outputdir/$run/nsources.out -sum 'kg/ha->Tg' -o $outputdir/$run/nsources_Tg.txt
aslice $outputdir/$run/fpc.out -o $outputdir/$run/fpc_all.txt
aslice $outputdir/$run/seasonality.out -o $outputdir/$run/seasonality_all.txt

time_slice=10
to="to"

first_year=$(awk 'FNR == 2 {print $1}' $outputdir/$run/cpool_Pg.txt)
last_year=$(awk 'END{print $1}' $outputdir/$run/cpool_Pg.txt)

if [[ $old_run != "_none_" ]]
then
    aslice $old_run/cflux.out -sum 'kg/m2->Pg' -o $outputdir/$ref/cflux_Pg.txt
    aslice $old_run/cpool.out -sum 'kg/m2->Pg' -o $outputdir/$ref/cpool_Pg.txt
    aslice $old_run/nsources.out -sum 'kg/ha->Tg' -o $outputdir/$ref/nsources_Tg.txt
    aslice $old_run/fpc.out -o $outputdir/$ref/fpc_all.txt
    aslice $old_run/seasonality.out -o $outputdir/$ref/seasonality_all.txt

    first_year_old=$(awk 'FNR == 2 {print $1}' $outputdir/$ref/cpool_Pg.txt)
    last_year_old=$(awk 'END{print $1}' $outputdir/$ref/cpool_Pg.txt)

    t_year=$(( last_year < last_year_old ? last_year : last_year_old ))
    start_year=$(( first_year > first_year_old ? first_year : first_year_old ))
    f_year=$(( t_year-time_slice > start_year ? t_year-time_slice : start_year ))
else
    t_year=$last_year
    f_year=$(( t_year-time_slice > start_year ? t_year-time_slice : start_year ))
fi

echo "Time slice comparison will be between years : " $f_year " and " $t_year
echo ""

tslice $outputdir/$run/cflux.out -f $f_year -t $t_year -o $outputdir/$run/cflux$f_year$to$t_year.txt
tslice $outputdir/$run/cpool.out -f $f_year -t $t_year -o $outputdir/$run/cpool$f_year$to$t_year.txt
tslice $outputdir/$run/fpc.out -f $f_year -t $t_year -o $outputdir/$run/fpc$f_year$to$t_year.txt
tslice $outputdir/$run/lai.out -f $f_year -t $t_year -o $outputdir/$run/lai$f_year$to$t_year.txt
tslice $outputdir/$run/nsources.out -f $f_year -t $t_year -o $outputdir/$run/nsources$f_year$to$t_year.txt
tslice $outputdir/$run/seasonality.out -f $f_year -t $t_year -o $outputdir/$run/seasonality$f_year$to$t_year.txt

if [[ $old_run != "_none_" ]]
then
    tslice $old_run/cflux.out -f $f_year -t $t_year -o $outputdir/$ref/cflux$f_year$to$t_year.txt
    tslice $old_run/cpool.out -f $f_year -t $t_year -o $outputdir/$ref/cpool$f_year$to$t_year.txt
    tslice $old_run/fpc.out -f $f_year -t $t_year -o $outputdir/$ref/fpc$f_year$to$t_year.txt
    tslice $old_run/lai.out -f $f_year -t $t_year -o $outputdir/$ref/lai$f_year$to$t_year.txt
    tslice $old_run/nsources.out -f $f_year -t $t_year -o $outputdir/$ref/nsources$f_year$to$t_year.txt
    tslice $old_run/seasonality.out -f $f_year -t $t_year -o $outputdir/$ref/seasonality$f_year$to$t_year.txt
fi

echo "Manipulation of data complete."; echo""
echo "Run dataset is between years : " $first_year " and " $last_year

if [[ $old_run != "_none_" ]]
then
    echo "Ref dataset is between years : " $first_year_old " and " $last_year_old
fi
echo ""
echo ""

matlab -nosplash -nodesktop -r "lpjg_analysis('$outputdir', '$f_year', '$t_year')"

echo "Finished!"
echo ""
