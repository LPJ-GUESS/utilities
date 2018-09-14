////////////////////////////////////////////////////////////////////////////////////////
// CLEAN
// Postprocessing utility for LPJ-GUESS
// Takes ASCII text file with or without header row as input file
// Describes data, identifies potential format errors, corrects minor format errors,
// adds header row
//
// Written by Ben Smith
// This version dated 2006-04-22
//
// clean -help for documentation 

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <gutil.h>

const int MAXITEM=380;
	// Maximum number of items in a record (row) of an output file

char logfile[]="clean.log";

class Record {

public:
	int nrec[MAXITEM];
	float maxval[MAXITEM];
	float val[MAXITEM];
	float minval[MAXITEM];
	bool valid[MAXITEM];
	
	Record() {
	
		int i;

		for (i=0;i<MAXITEM;i++) {
			nrec[i]=0;
			val[i]=0.0;
			valid[i]=true;
		}
	}
	
	void add_record(Record& rec) {
	
		// Function to add another record to this record
		
		int i;

		for (i=0;i<MAXITEM;i++) {
			if (rec.valid[i]) {
				val[i]+=rec.val[i];
				if (nrec[i]) {
					if (rec.val[i]>maxval[i]) maxval[i]=rec.val[i];
					if (rec.val[i]<minval[i]) minval[i]=rec.val[i];
				}
				else {
					maxval[i]=rec.val[i];
					minval[i]=rec.val[i];
				}
				nrec[i]++;
			}
		}
	}
	
	void average() {
	
		// Calculates average over number of added records
		
		int i;
		for (i=0;i<MAXITEM;i++) 
			if (nrec[i]) val[i]/=(double)nrec[i];
	}
};

class Item {

public:
	xtring label,fmt,lfmt;
	bool ifnum;
	bool ifsign;
	bool ifalpha;
	bool exclude;
	int places;
	int digits;
	int width;
	
	Item() {
		label="";
		ifnum=true;
		ifalpha=false;
		ifsign=false;
		exclude=false;
		places=digits=0;
	}
	
	void compute_fmt() {
	
		int w;
		
		if (digits && digits<12) w=digits;
		else w=1;
		
		if (places && places<12) w+=places+1;
		if (ifsign) w++;
		
		if (w>21) {
			ifnum=false;
			w=21;
		}
		
		if (label.len()>w) w=label.len();
		
		if (ifnum)
			fmt.printf("%%%d.%df",w,places);
		else {
			if (w<8) w=8;
			fmt.printf("%%%dg",w);
		}
		
		lfmt.printf("%%%ds",w);
		
		width=w;
	}
};


bool scanitem(xtring text,int& places,int& digits,bool& ifsign) {

	places=0;
	digits=0;
	int i,ifdecimal=0,placepos=0;
	bool ifnum=true;
	ifsign=false;
	char ch;
		
	i=0;
	ch=text[i];
	while (ch && ifnum) {
		
		if (ch=='0') {
			if (!ifdecimal) digits++;
		}
		else if (ch>'0' && ch<='9') {
			if (ifdecimal) placepos=i+1;
			else digits++;
		}
		else if ((ch=='-' || ch=='+') && !ifsign) ifsign=true;
		else if (ch=='.' && !ifdecimal) ifdecimal=i+1;
		else {
			//printf("Falsifying ifnum on %s\n",(char*)text);
			ifnum=false;
		}
		i++;
		ch=text[i];
	}
	
	if (ifdecimal && placepos) places=placepos-ifdecimal;
	else places=0;
		
	return ifnum;
}


bool readheader(FILE*& in,Item* items,int& ncol,xtring filename,
	double values[MAXITEM],bool& ifvalues,int& lineno,bool& ifdos) {

	// Reads header row of an LPJ-GUESS output file
	// label = array of header labels
	// ncol  = number of columns (labels)
	// Returns false if too many items in file
	
	xtring line,item;
	int pos,i;
	bool alphabetics=false;
	ncol=0;
	ifdos=false;
	const int MAXLEN=63;
	
	while (!ncol && !feof(in)) {
	
		readfor(in,"a#",&line);
		lineno++;
		
		if (line.findoneof("\r")!=-1) ifdos=true;
			
		pos=line.findnotoneof(" \t\r");
		while (pos!=-1) {
			line=line.mid(pos);
			pos=line.findoneof(" \t\r");
			if (ncol>MAXITEM) {
				printf("Too many columns (>%d) in %s\n",MAXITEM,(char*)filename);
				return false;
			}
			if (pos>0) {
				item=line.left(pos);
				if (item.len()>MAXLEN) item=item.left(MAXLEN);
				items[ncol].label=item;
				line=line.mid(pos);
				pos=line.findnotoneof(" \t\r");
			}
			else {
				item=line;
				if (item.len()>MAXLEN) item=item.left(MAXLEN);
				items[ncol].label=item;
			}
			
			if (item.isnum()) {
				values[ncol]=item.num();
				scanitem(item,items[ncol].places,items[ncol].digits,items[ncol].ifsign);
			}
			else
				alphabetics=true;
			
			ncol++;
			
		}
	}
	
	if (!ncol) {
		printf("%s contains no data\n",(char*)filename);
		return false;
	}

	if (!alphabetics) {
	
		for (i=0;i<ncol;i++) {
			items[i].label.printf("Column%d",i+1);
		}
		
		ifvalues=true;
	}
	else ifvalues=false;

	return true;
}

bool readrecord(FILE*& in,FILE*& out,Record& rec,int nitem,Item* items,
	xtring sfmt,xtring dfmt,int& nrec,int& lineno,xtring& filename,
	int& nblank,int& nirreg,int& nalpha,xtring& fmt,bool& ifdos,int noutitem) {

	// Reads one record (row) in output file
	// Returns false on end of file
	
	xtring sval[MAXITEM],s[MAXITEM];
	bool valid[MAXITEM];
	double dval[MAXITEM];
	int i,places,digits,n,m,validitems;
	bool ifsign,searching=true,isnum;
	
	for (i=0;i<MAXITEM;i++) valid[i]=false;

	while (searching) {
		
		n=0;
		m=0;
		validitems=0;
		isnum=true;
		if (!readfor(in,fmt,&s)) return false;
		while (s[n]!="" && n<MAXITEM) {
			if (s[n].find("\r")!=-1) {
				ifdos=true;
				s[n]=s[n].left(s[n].find("\r"));
			}
			if (s[n]!="") {
				sval[n]=s[n];
				if (!items[n].exclude) {
					if (!s[n].isnum()) {
						isnum=false;
						valid[n]=false;
					}
					else {
						valid[n]=true;
						validitems++;
					}
					m++;
				}
				n++;
			}
		}

		lineno++;

		if (!isnum && m==noutitem) {
			if (out) fprintf(out,"Line %d contains non-numeric data - ignoring %d item(s)\n",
				lineno,noutitem-validitems);
			nalpha++;
		}
		
		if (m!=noutitem) {

			if (m>noutitem) {
				if (out) fprintf(out,"Line %d contains more than %d items\n",lineno,noutitem);
				nirreg++;
			}
			else if (n) {
				if (out) fprintf(out,"Line %d contains fewer than %d items\n",lineno,noutitem);
				nirreg++;
			}
			else {
				if (out) fprintf(out,"Line %d is blank - ignoring\n",lineno);
				nblank++;
			}
		}
		
		if (n) {
			for (i=0;i<nitem;i++) {
				if (!items[i].exclude) {
					if (valid[i]) {
						if (scanitem(sval[i],places,digits,ifsign)) {
							if (places>items[i].places) items[i].places=places;
							if (digits>items[i].digits) items[i].digits=digits;
							if (ifsign) items[i].ifsign=true;
						}
						else items[i].ifnum=false;
						rec.val[i]=sval[i].num();
						rec.valid[i]=true;
					}
					else {
						rec.valid[i]=false;
						items[i].ifalpha=true;
					}
					searching=false;
				}
			}
			nrec++;
		}
	}
	
	return true;
}

bool tagitems(xtring filename,Item* items,int nitem,xtring exclude[MAXITEM],
	int excludeno[MAXITEM],int nexclude,int& noutitem) {

	int i,j,k;
	bool found;
	
	//printf("nexclude=%d\n",nexclude);
	
	for (i=0;i<nexclude;i++) {
		if (exclude[i]!="") {
			found=false;
			for (j=0;j<nitem;j++) {
				if (items[j].label==exclude[i]) {
					k=j;
					found=true;
					j=nitem;
				}
			}
			if (!found) {
				found=false;
				for (j=0;j<nitem;j++) {
					if (items[j].label.lower()==exclude[i].lower()) {
						k=j;
						found=true;
						j=nitem;
					}
				}
				if (found) {
					printf("Item %s not found in %s - excluding %s instead\n",
						(char*)exclude[i],(char*)filename,(char*)items[k].label);
				}
				else {
					printf("Item %s not found in %s\n",(char*)exclude[i],(char*)filename);
					return false;
				}
			}
			items[k].exclude=true;
		}
		else if (excludeno[i]) {
			if (excludeno[i]>nitem) {
				printf("Item number %d not found in %s\n",excludeno[i],(char*)filename);
				return false;
			}
			items[excludeno[i]-1].exclude=true;
		}
	}
	
	noutitem=0;
	for (i=0;i<nitem;i++) {
		//if (items[i].exclude) printf("Excluding %d:%s\n",i,(char*)items[i].label);
		if (!items[i].exclude) noutitem++;
	}

	return true;
}

bool writerec(FILE*& out,Item* items,int nitem,Record& rec,char* sep) {
	
	int i,j;
	bool first,valid;

	
	valid=true;
	for (i=0;i<nitem;i++) {
		if (!items[i].exclude) {
			if (!rec.valid[i]) {
				valid=false;
				i=nitem;
			}
		}
	}
	
	if (valid) {
	
		// Print data
		
		first=true;
		for (j=0;j<nitem;j++) {
			if (!items[j].exclude) {
				if (!first) fprintf(out,sep);
				fprintf(out,items[j].fmt,(double)rec.val[j]);
				first=false;
			}
		}
		
		fprintf(out,"\n");
	}
	
	return true;
}


bool readdata(FILE*& outlog,xtring filename,xtring outfile,Item* items,int& nitem,int& nrec,
	Record& sumrec,
	int& nblank,int& nirreg,int& nalpha,bool& ifheader,bool& iflog,bool& ifdos,bool ifwriting,
	xtring exclude[MAXITEM],int excludeno[MAXITEM],int nexclude,int& noutitem,xtring sep,
	xtring* header,int nheader) {

	FILE* out;
	int recno,i,j;
	double dval[MAXITEM];
	bool ifvalues,first,incl;
	int lineno=0;
	xtring sfmt,dfmt,rfmt,banner;
	Record thisrec;
	nrec=0;
	
	FILE* in=fopen(filename,"rt");
	if (!in) {
		printf("Could not open %s for input\n",(char*)filename);
		return false;
	}
	
	if (ifwriting) {
		out=fopen(outfile,"wt");
		if (!out) {
			printf("Could not open %s for output\n",(char*)outfile);
			return false;
		}
		
		printf("Writing clean data to %s ...\n",(char*)outfile);
		
		first=true;
		for (i=0;i<nitem;i++) {
			if (!items[i].exclude) {
				if (!first) fprintf(out,(char*)sep);
				fprintf(out,items[i].lfmt,(char*)items[i].label);
				first=false;
			}
		}
		fprintf(out,"\n");
	}

	unixtime(banner);
	if (!ifwriting) printf("Reading data from %s ...\n",(char*)filename);
	if (outlog) fprintf(outlog,"In %s on %s:\n",(char*)filename,(char*)banner);
	
	// Read header
	if (!readheader(in,items,nitem,filename,dval,ifvalues,lineno,ifdos)) {
		return false;
	}
	
	if (ifvalues) {
		for (i=0;i<MAXITEM;i++) {
			if (i<nitem) {
				thisrec.val[i]=dval[i];
				thisrec.valid[i]=true;
			}
			else thisrec.valid[i]=false;
		}
	}

	if (!tagitems(filename,items,nitem,exclude,excludeno,nexclude,noutitem))
		return false;

	// Replace with column labels specified on command line
	
	if (nheader) {
		j=0;
		for (i=0;i<nheader;i++) {
			while (items[j].exclude && j<MAXITEM) j++;
			if (j>=MAXITEM) {
				printf("Too many items\n");
				return false;
			}
			items[j].label=header[i];
			j++;
		}
		
		nitem=j;
		noutitem=nheader;
	}
	
	sfmt.printf("%da",nitem);
	dfmt.printf("%df",nitem);
	rfmt.printf("%da",MAXITEM);
	
	nblank=lineno-1;
	if (nblank && outlog) fprintf(outlog,"Lines 1-%d are blank (ignoring)\n",nblank);

	// Transfer data from first row (if all numbers)
	
	if (ifvalues) {
	
		for (i=0;i<nitem;i++) {
			if (!items[i].exclude) {
				if (!thisrec.valid[i] && outlog) {
					fprintf(outlog,"Line %d contains fewer than %d items\n",lineno,noutitem);
					nirreg=1;
				}
			}
		}
	
		if (ifwriting) writerec(out,items,nitem,thisrec,sep);
		else {
			sumrec.add_record(thisrec);
			nrec++;
		}
		
		ifheader=false;
	}
	else ifheader=true;
		
	while (!feof(in)) {
		
		// Read next record in file
		
		if (readrecord(in,outlog,thisrec,nitem,items,sfmt,dfmt,nrec,lineno,filename,
			nblank,nirreg,nalpha,rfmt,ifdos,noutitem)) {
			
			if (ifwriting)
				writerec(out,items,nitem,thisrec,sep);
			else
				sumrec.add_record(thisrec);
		}
	}
	
	// Now calculate averages for timeslice
	
	sumrec.average();
	
	for (i=0;i<nitem;i++) {

		items[i].compute_fmt();
	}
	
	fclose(in);
	
	if (ifwriting) {
	
		printf("\n");
		fclose(out);
	}
	
	return true;
}

void printstats(FILE* out,xtring filename,Item* items,Record& rec,int nitem,int nrec,
	int nblank,int nirreg,int nalpha,bool ifheader,bool iflog,bool ifdos,int noutitem) {

	int i,j,k,w;
	bool ifnum=true,newline=false;
	xtring ifmt[MAXITEM];
	
	const int WIDTH=80;
	
	for (i=0;i<nitem;i++)
		ifmt[i].printf("%%%dd",items[i].width);

	if (!ifheader || nblank || nalpha || nirreg || ifdos) {
	
		fprintf(out,"\n***********************************************************************\n");
		fprintf(out,"%s has possible format error(s):\n",(char*)filename);
		if (!ifheader) fprintf(out,"  - missing header row with item labels\n");
		if (nblank) fprintf(out,"  - %d blank line(s) (ignored)\n",nblank);
		if (nalpha) fprintf(out,"  - %d line(s) containing non-numeric data (non-numeric items ignored)\n",nalpha);
		if (nirreg) fprintf(out,"  - %d line(s) containing more or less than %d items\n",
			nirreg,noutitem);
		if (ifdos) {
			fprintf(out,"  - appears to be in Windows/DOS ASCII format - use ASCII mode for FTP\n");
			fprintf(out,"    transfer and/or convert with dos2unix\n");
		}
		if (iflog) fprintf(out,"Details are available in %s\n",logfile);
		fprintf(out,"***********************************************************************\n");
	}
	
	if (nalpha)
		fprintf(out,"\nRead %d full or part records for %d items\n",nrec,noutitem);
	else
		fprintf(out,"\nRead %d records for %d items\n",nrec,noutitem);
	
	newline=true;
	i=0;
	while (i<nitem) {
		
		j=i;
		w=5;
		while (w<WIDTH && i<nitem) {
			if (!items[i].exclude) w+=items[i].width+2;
			if (w<WIDTH && i<nitem) i++;
		}

		for (k=j;k<i;k++) {
			if (!items[k].exclude) {
				if (newline) fprintf(out,"\n     ");
				newline=false;
				if (items[k].ifalpha) {
					fprintf(out," ");
					ifnum=false;
				}
				else fprintf(out,"  ");
				fprintf(out,items[k].lfmt,(char*)items[k].label);
				if (items[k].ifalpha) fprintf(out,"*");
			}
		}
		fprintf(out,"\nMax  ");
		for (k=j;k<i;k++) {
			if (!items[k].exclude) {
				fprintf(out,"  ");
				fprintf(out,items[k].fmt,rec.maxval[k]);
			}
		}
		fprintf(out,"\nMean ");
		for (k=j;k<i;k++) {
			if (!items[k].exclude) {
				fprintf(out,"  ");
				fprintf(out,items[k].fmt,rec.val[k]);
			}
		}
		fprintf(out,"\nMin  ");
		for (k=j;k<i;k++) {
			if (!items[k].exclude) {
				fprintf(out,"  ");
				fprintf(out,items[k].fmt,rec.minval[k]);
			}
		}
		if (nalpha || nirreg) {
			fprintf(out,"\nnrec+");
			for (k=j;k<i;k++) {
				if (!items[k].exclude) {
					fprintf(out,"  ");
					fprintf(out,ifmt[k],rec.nrec[k]);
				}
			}
		}
		fprintf(out,"\n");
		newline=true;
	}
	if (!ifnum) fprintf(out,"\n* Item may have missing values or non-numeric data");
	if (nalpha || nirreg) fprintf(out,"\n+ Records containing numeric data for this item");
	if (!ifnum || nalpha || nirreg) fprintf(out,"\n");
}



void stripfilename(xtring& text) {

	// Extracts file part (no extension or directory part) from a pathname
	
	int i;
	
	i=text.len()-1;
	while (i>=0) {
		if (text[i]=='/' || text[i]=='\\') {
			text=text.mid(i+1);
			i=0;
		}
		i--;
	}
	
	i=0;
	while (i<text.len()) {
		if (text[i]=='.') {
			text=text.left(i);
			i=text.len();
		}
		i++;
	}
}

void helptext(FILE* out,xtring exe) {
	
	fprintf(out,"Usage: %s <input-file> <options>\n\n",(char*)exe);
	fprintf(out,"Options:\n\n");
	fprintf(out,"-o <output-file>\n");
	fprintf(out,"    Pathname for output file with cleaned-up data\n");
	fprintf(out,"-n\n");
	fprintf(out,"    Suppresses output of cleaned data file\n");
	fprintf(out,"-tab\n");
	fprintf(out,"    Tab-delimited output\n");
	fprintf(out,"-x <item-name> | <column-number> { <item-name> | <column-number> }\n");
	fprintf(out,"    List of items or 1-based column numbers in input file to exclude from input\n");
	fprintf(out,"    and output. Item names must correspond to labels in input file header,\n");
	fprintf(out,"    not new/changed labels specified with -h.\n");
	fprintf(out,"-h <item-name> { <item-name> }\n");
	fprintf(out,"    Item labels for inclusion in header of clean output file. Number of labels\n");
	fprintf(out,"    should equal number of columns in cleaned output file, taking into account\n");
	fprintf(out,"    items excluded with -x\n");
	fprintf(out,"-help\n");
	fprintf(out,"    Displays this help message\n");
}


void printhelp(xtring exe) {

	helptext(stdout,exe);

	FILE* out=fopen("usage.txt","wt");
	if (out) {
		helptext(out,exe);
		printf("\nHelp message is also available in the file usage.txt in this directory\n");
		fclose(out);
	}

	exit(99);
}

void abort(xtring exe) {

	printf("Usage: %s <input-file> <options>\n",(char*)exe);
	printf("Options: -o <output-file>\n");
	printf("         -n\n");
	printf("         -tab\n");
	printf("         -x <item-name> | <column-number> { <item-name> | <column-number> }\n");
	printf("         -h <item-name> { <item-name> }\n");
	printf("         -help\n");

	exit(99);
}

bool processargs(int argc,char* argv[],xtring& infile,xtring& outfile,
	xtring exclude[MAXITEM],int excludeno[MAXITEM],int& nexclude,
	xtring& sep,xtring header[MAXITEM],int& nheader) {

	int i,itemno;
	xtring arg,item;
	bool slut,havefile=false;
	bool ifoutput=true;
	double dval;
	
	// Defaults
	sep=" ";
	outfile="";
	nheader=0;
	for (i=0;i<MAXITEM;i++) {
		exclude[i]="";
		excludeno[i]=0;
	}
	nexclude=0;
	
	for (i=1;i<argc;i++) {
		arg=argv[i];
		if (arg[0]=='-') {
			arg=arg.lower();
			if (arg=="-o") { // output file
				if (argc>=i+2) {
					outfile=argv[i+1];
				}
				else {
					printf("Option -o must be followed by output file name or path\n");
					return false;
				}
				i+=1;
			}
			else if (arg=="-h") { // header item list
				slut=false;
				while (argc>=i+2 && !slut) {
					item=argv[i+1];
					if (item[0]=='-') slut=true;
					else {
						if (nheader==MAXITEM) {
							printf("Option -h: too many items (maximum allowed is %d)\n",MAXITEM);
							return false;
						}
						header[nheader++]=item;
						i++;
					}
				}
			}
			else if (arg=="-x") {
				// column label or item number to omit in output file
				
				slut=false;
				while (argc>=i+2 && !slut) {
					item=argv[i+1];
					if (item[0]=='-') slut=true;
					else {
						if (item.isnum()) {
							dval=item.num();
							if (dval>=1.0 && int(dval)==dval) { // seems to be an item number
								itemno=dval;
								if (itemno>MAXITEM) {
									printf("Option -x: too large column number (maximum allowed is %d)\n",MAXITEM);
									return false;
								}
								if (nexclude==MAXITEM) {
									printf("Option -x: too many items (maximum allowed is %d)\n",MAXITEM);
									return false;
								}
								excludeno[nexclude++]=itemno;
							}
						}
						else {
							if (nexclude==MAXITEM) {
								printf("Option -x: too many items (maximum allowed is %d)\n",MAXITEM);
								return false;
							}
							exclude[nexclude++]=item;
						}
						i++;
					}
				}
			}
			else if (arg=="-tab") {
				sep="\t";
			}
			else if (arg=="-n") {
				ifoutput=false;
			}
			else if (arg=="-h" || arg=="-help") printhelp(argv[0]);
			else {
				printf("Invalid option %s\n",(char*)arg);
				return false;
			}
		}
		else {
			if (havefile) {
				printf("Only one input file may be specified\n");
				return false;
			}
			else {
				infile=arg;
				havefile=true;
			}
		}
	}
			
	if (!havefile) {
		printf("Input file name or path must be specified\n");
		return false;
	}
	
	if (outfile=="") {
		
		xtring filepart=infile;
		stripfilename(filepart);
		
		if (ifoutput) outfile.printf("%s_clean.txt",(char*)filepart);
	}
	else if (!ifoutput) {
		printf("Options -o and -n not allowed together - do you want output or not?\n");
		return false;
	}

	if (outfile==infile) { 
		printf("Cannot overwrite %s\n",(char*)infile);
		return false;
	}
	
	return true;
}



int main(int argc,char* argv[]) {

	xtring infile,outfile,banner,header[MAXITEM];
	bool ifheader,iflog,ifdos,ifoutput;
	Item items[MAXITEM];
	int nitem,nrec,nheader;
	int nblank=0,nirreg=0,nalpha=0;
	xtring sep;
	Record rec;
	xtring exclude[MAXITEM];
	int excludeno[MAXITEM];
	int nexclude,noutitem;
	
	if (!processargs(argc,argv,infile,outfile,exclude,excludeno,nexclude,
		sep,header,nheader))
			abort(argv[0]);

	unixtime(banner);
	banner=(xtring)"[CLEAN  "+banner+"]\n\n";
	printf("%s",(char*)banner);
	
	FILE* outlog=fopen(logfile,"wt");
	if (!outlog) {
		printf("Warning: Could not open %s for output\n\n",(char*)logfile);
		iflog=false;
	}
	else iflog=true;

	if (readdata(outlog,infile,outfile,items,nitem,nrec,rec,nblank,nirreg,nalpha,ifheader,
		iflog,ifdos,false,exclude,excludeno,nexclude,noutitem,sep,header,nheader)) {

		printstats(stdout,infile,items,rec,nitem,nrec,nblank,nirreg,nalpha,
			ifheader,iflog,ifdos,noutitem);
		if (iflog) {
			printstats(outlog,infile,items,rec,nitem,nrec,nblank,nirreg,nalpha,
				ifheader,false,ifdos,noutitem);
		}
		
		if (outlog) {
			printf("\nDiagnostic output is available in %s\n\n",logfile);
			fclose(outlog);
		}
		
		if (outfile!="") {
			outlog=NULL;
			readdata(outlog,infile,outfile,items,nitem,nrec,rec,nblank,nirreg,nalpha,ifheader,
				iflog,ifdos,true,exclude,excludeno,nexclude,noutitem,sep,header,nheader);
		}
	}
	
	return 0;
}
