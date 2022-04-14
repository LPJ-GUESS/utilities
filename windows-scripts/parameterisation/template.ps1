
<#
An example of modifying the variable K_century_max in steps.
The template file, template.inz has the following:
!BEGIN template.inz
! pools SURFSTRUCT, SOILSTRUCT, SOILMICRO, SURFHUMUS, SURFMICRO, SURFMETA, SURFFWD, SURFCWD, SOILMETA, SLOWSOM, PASSIVESOM
!K_century_max 0.022 0.027 0.05 0.00027 0.04 0.08 0.011 0.0022 0.1 0.00027 0.0000042
K_century_max 0.022 0.027 0.05 0.00027 0.04 0.08 0.011 0.0022 0.1 SLOW PASS

outputdirectory "./OUTDIR/"

!END template.inz

The script will create a folder, modify the template file, run the model, and copy the modified file to the output-folder.
Include tmp.ins in the end of your main ins-file:  
import "tmp.ins"
in e.g. guess.ins as in the example below.
#>


#Ranges to change the parameters
$K_SLOW = @(0.00017,0.00022,0.00027,0.00037)
$K_PASS = @(0.0000019,0.00000305,0.0000042,0.00000535)


for ($p=0; $p -lt $K_PASS.length; $p++) {
	for ($s=0; $s -lt $K_SLOW.length; $s++) {
		$dir = "PASS_"+$p+"_SLOW_"+$s
		#Remove old directories if they exist
		if (Test-Path $dir) {
			Remove-Item -LiteralPath "$dir" -Force -Recurse
		}
		#Remove any old files
		if (Test-Path tmp.txt) {
			Remove-Item -LiteralPath tmp.* -Force -Recurse
		}

		New-Item -Path "." -Name "$dir" -ItemType "directory" -Force
	
		(Copy-Item template.ins tmp.txt -Force)
		
		((Get-Content -path tmp.txt -Raw) -replace 'SLOW',$K_SLOW[$s]) | Set-Content -Path tmp.txt
		((Get-Content -path tmp.txt -Raw) -replace 'PASS',$K_PASS[$p]) | Set-Content -Path tmp.txt
		((Get-Content -path tmp.txt -Raw) -replace 'OUTDIR',$dir) | Set-Content -Path tmp.txt
		
		Copy-Item tmp.txt tmp.ins -Force
		Copy-Item tmp.txt $dir/tmp.ins -Force
		
		#Add line to run LPJ-GUESS, for example:
		#.\guesscmd.exe guess.ins
	}
}