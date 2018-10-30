# ec-earth

# A simple LPJ-GUESS analysis tool for the EC-Earth community

Need to have all three files in the same folder, have matlab installed (set to tetralith matlab in lpjg_fast_analysis.sh at the moment) and have LPJ-GUESS standard tools installed (aslice and tslice, installed on tetralith)

# Usage: lpjg_fast_analysis.sh <PATH_TO_RUNDIR> [-o <OUTPATH>] [-r <REFPATH>] [fwah]"

PATH_TO_RUNDIR: should be to the level <rundir>/<exp-name>/output/lpjg"

# Options:
  -r --reference       <PATH> to a dir with aggregated LPJG reference run"
  -o --outdir          <PATH> to store output in, DEFAULT='./'"
  -f --force_aggregate -> do exactly that, in case a previous aggregation has failed..."
  -w --overwrite       if outdir exists force overwriting of analysis data"
  -a --aggregate_all   aggregate the full set of LPJG output files"
  -h --help            help
