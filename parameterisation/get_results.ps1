
<#
An example of retreiving the results 
that are the result of the run_parameters.ps1
script.
#>

$FILE = "cpool"
$IFILE = $FILE+".out"
$OFILE = $FILE+".dat"
#Ranges to change the parameters
$K_SLOW = @(0.00017,0.00022,0.00027,0.00037)
$K_PASS = @(0.0000019,0.00000305,0.0000042,0.00000535)
#Remove the old result file
if (Test-Path $OFILE) {
	Remove-Item -LiteralPath $OFILE -Force -Recurse
}

for ($p=0; $p -lt $K_PASS.length; $p++) {
	for ($s=0; $s -lt $K_PASS.length; $s++) {
		$dir = "PASS_"+$p+"_SLOW_"+$s
		
		#Remove old directories if they exist
		if (Test-Path $dir) {
			#takes line 53 and adds it to the result file
			$p+" " | Out-File -append -encoding ascii $OFILE -NoNewline
			$s | Out-File -append -encoding ascii $OFILE -NoNewline
			(Get-Content -Path $dir/$IFILE)[52] | Add-Content -Path $OFILE
		}
	
	}
}