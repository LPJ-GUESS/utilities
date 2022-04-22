////////////////////////////////////////////////////////////////////////////////////////
// JOYN
// Postprocessing utility for LPJ-GUESS
// Takes two raw or postprocessed ASCII output files from LPJ-GUESS as input files
// Generates output file with items from matching records in both files
//
// Written by Ben Smith
// This version dated 2006-06-07
//
// joyn -help for documentation 

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <gutil.h>

const int MAXINDEX=8;
	// Maximum number of index items (e.g. longitude, latitude, year)
const int MAXITEM=60;
	// Maximum number of items in a record (row) of an input file
const int MAXGRID=65000;
	// Maximum number of grid cells / records in input files

class Record {

public:
	int nrec;
	float val[MAXITEM];
	
	Record() {
	
		int i;
		
		nrec=0;
		
		for (i=0;i<MAXITEM;i++) val[i]=0.0;
	}
};

class Item {

public:
	xtring label,fmt,lfmt;
	bool ifnum;
	bool ifsign;
	int indexitemno;
	int places;
	int digits;
	int colno[2];
	
	Item() {
		label="";
		ifnum=true;
		ifsign=false;
		indexitemno=-1; // signifies "not an index item"
		places=digits=0;
	}
	
	void compute_fmt() {
	
		int w;
		
		if (digits) w=digits;
		else w=1;
		
		if (places) w+=places+1;
		if (ifsign) w++;
		
		w+=1;
		
		if (label.len()>w) w=label.len();
		
		if (ifnum)
			fmt.printf("%%%d.%df",w,places);
		else fmt.printf("%%%dg",w);
		
		lfmt.printf("%%%ds",w);
	}
};

// Global matrix to store data from files
Record data[MAXGRID];

bool scanitem(xtring text,int& places,int& digits,bool& ifsign) {

	places=0;
	digits=0;
	int i;
	bool ifnum=true;
	ifsign=false;
	bool ifdecimal=false;
	char ch;
		
	i=0;
	ch=text[i];
	while (ch && ifnum) {
		
		if (ch>='0' && ch<='9') {
			if (ifdecimal) places++;
			else digits++;
		}
		else if ((ch=='-' || ch=='+') && !ifsign) ifsign=true;
		else if (ch=='.' && !ifdecimal) ifdecimal=true;
		else {
			//printf("Falsifying ifnum on %s\n",(char*)text);
			ifnum=false;
		}
		i++;
		ch=text[i];
	}
		
	return ifnum;
}


bool readheader(FILE*& in,Item* items,int& ncol,int& nindexitem,
	xtring indexitem[MAXINDEX],int indexitemno[MAXINDEX],xtring filename,int fileno,
	double values[MAXITEM],bool& ifvalues,bool relax,int& lineno) {

	// Reads header row of an LPJ-GUESS output file
	// label = array of header labels
	// ncol  = number of columns (labels)
	// Returns false if too many items in file
	
	xtring line,item;
	int pos,i,j,idindex[MAXINDEX],lonpos,latpos;
	bool alphabetics=false;
	ncol=0;

	for (i=0;i<nindexitem;i++) idindex[i]=-1;
	lonpos=-1;
	latpos=-1;

	while (!ncol && !feof(in)) {
	
		readfor(in,"a#",&line);
		lineno++;

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
				items[ncol].label=item;
				line=line.mid(pos);
				pos=line.findnotoneof(" \t\r");
			}
			else {
				item=line;
				items[ncol].label=item;
			}

			if (item.isnum()) {
				values[ncol]=item.num();
				scanitem(item,items[ncol].places,items[ncol].digits,items[ncol].ifsign);
			}
			else {
				for (i=0;i<nindexitem;i++) {
					if (idindex[i]<0 && item.lower()==indexitem[i].lower()) {
						idindex[i]=ncol+1;
					}
				}
				if (item.len()>=3) {
					if (item.left(3).lower()=="lon" && lonpos<0) lonpos=ncol+1;
					if (item.left(3).lower()=="lat" && latpos<0) latpos=ncol+1;
				}
				alphabetics=true;
			}
			
			for (i=0;i<nindexitem;i++)
				if (idindex[i]<0 && indexitemno[i]==ncol+1) idindex[i]=ncol+1;
				
			ncol++;
		}
	}

	for (i=0;i<nindexitem;i++) {
		if (idindex[i]<0) {
			if (indexitem[i]=="") {
				printf("Index item %d not found in %s (has only %d columns)\n",
					indexitemno[i],(char*)filename,ncol);
				return false;
			}	
			else if (!(relax && !alphabetics)) {
				printf("Index item %s not found in %s\n",(char*)indexitem[i],
					(char*)filename);
				return false;
			}
		}

		items[idindex[i]-1].indexitemno=i;
	}
	
	if (!alphabetics) {
	
		for (i=0;i<ncol;i++) {
			items[i].label.printf("Col%d.%d",fileno+1,i+1);
		}
		
		ifvalues=true;
	}
	else ifvalues=false;
	
	if (nindexitem==0) {
		if (alphabetics) {
			if (lonpos<0)
				printf("Default index item longitude (Lon) not found in %s\n",(char*)filename);
			if (latpos<0)
				printf("Default index item latitude (Lat) not found in %s\n",(char*)filename);
			if (lonpos<0 || latpos<0) return false;
			items[lonpos-1].indexitemno=0;
			items[latpos-1].indexitemno=1;
		}
		else {
			items[0].indexitemno=0;
			items[1].indexitemno=1;
			items[0].label="Lon";
			items[1].label="Lat";
		}
	}
	
	if (!ncol) {
		printf("%s contains no data\n",(char*)filename);
		return false;
	}
	else if (ncol<2) {
		printf("At least three columns expected in %s\n",(char*)filename);
		return false;
	}

	return true;
}

bool readrecord(FILE*& in,Record& rec,int ncol,int nitem,Item* items,xtring sfmt,xtring dfmt,
	int nrec,bool iffast,int fileno,int& lineno,xtring& filename) {

	// Reads one record (row) in output file
	// Returns false on end of file
	// ncol = total number of items/columns in this file including index items
	// nitem = number of items to assign data to
	// fileno = file number
	
	xtring sval[MAXITEM],sval_curr,whole_line,line;
	double dval[MAXITEM];
	int i,places,digits,pos;
	bool ifsign,searching=true,blank,isnum;

	while (searching) {
	
		if (nrec<100 || !(nrec%10) || !iffast) {

			if (!readfor(in,"a#",&whole_line)) return false;
			if (feof(in)) return false;
			line=whole_line;
			lineno++;
			i=0;
			
			blank=true;
			isnum=true;
			pos=line.findnotoneof(" \t\r");
			while (pos!=-1 && i<nitem) {
				line=line.mid(pos);
				blank=false;
				pos=line.findoneof(" \t\r");
				if (pos>0) {
					sval[i]=line.left(pos);
					line=line.mid(pos);
					pos=line.findnotoneof(" \t\r");
				}
				else {
					sval[i]=line;
				}
				if (!sval[i].isnum()) {
					isnum=false;
					i=nitem;
				}
				i++;
			}

			if (blank)
				printf("Line %d of %s is blank - ignoring\n",lineno,(char*)filename);
			else {
				for (i=0;i<nitem;i++) {
				
					if (items[i].colno[fileno]!=-1) {
						sval_curr=sval[items[i].colno[fileno]];
						if (!sval_curr.isnum()) {
							printf("Line %d of %s contains non-numeric data - ignoring entire line\n",
								lineno,(char*)filename);
							searching=true;
							i=nitem;
						}
						else {
							if (scanitem(sval_curr,places,digits,ifsign)) {
								if (places>items[i].places) items[i].places=places;
								if (digits>items[i].digits) items[i].digits=digits;
								if (ifsign) items[i].ifsign=true;
							}
							else items[i].ifnum=false;
							rec.val[i]=sval_curr.num();
							searching=false;
						}
					}
				}
			}
		}
		else {
			if (!readfor(in,dfmt,dval)) return false;
			lineno++;
			for (i=0;i<nitem;i++)
				if (items[i].colno[fileno]!=-1)
					rec.val[i]=dval[items[i].colno[fileno]];
			searching=false;
		}
	}

	rec.nrec=1;
	
	return true;
}

bool finditem(xtring item,int& itemno,Item* items,int nitem) {

	int i;
	
	itemno=-1;
	for (i=0;i<nitem;i++) {
		if (items[i].label==item) {
			itemno=i;
			return true;
		}
	}
	if (itemno==-1) {
	
		// Not found - try case insensitive comparison
		
		itemno=-1;
		for (i=0;i<nitem;i++) {
			if (items[i].label.lower()==item.lower()) {
				itemno=i;
				return true;
			}
		}
	}
	
	return false;
}

int findrec(Record& rec,int nrec,Item* items,int nitem,int fileno,int guess) {

	// Returns record number in global data corresponding to given values of index items
	// in argument record
	// -1 if not found
	// nrec = number of records in global data
	// guess = guess at matching record number
	
	int i,j;
	bool matches;
	
	if (guess<nrec) {
		matches=true;
		for (j=0;j<nitem && matches;j++) {
			if (items[j].indexitemno!=-1) {
				if (rec.val[items[j].colno[fileno]]!=data[guess].val[j]) matches=false;
			}
		}
		
		if (matches) return guess;
	}
	
	// Not found at that position
	
	for (i=nrec-1;i>=0;i--) {
		matches=true;
		for (j=0;j<nitem && matches;j++) {
			if (items[j].indexitemno!=-1) {
				if (rec.val[items[j].colno[fileno]]!=data[i].val[j]) matches=false;
			}
		}
		
		if (matches) return i;
	}
		
	return -1;
}

bool readwritedata(xtring infile1,xtring infile2,xtring outfile,Item* items,int& nitem,
	xtring indexitem[MAXINDEX],int indexitemno[MAXINDEX],int& nindexitem,
	int& nrec,bool iffast,int& lonely_recs,char* sep) {

	int recno,i,j,nrec1;
	double dval1[MAXITEM],dval2[MAXITEM];
	bool ifvalues1,ifvalues2,warned;
	xtring sfmt,dfmt;
	Record rec;
	Item items1[MAXITEM],items2[MAXITEM];
	int nitem1,nitem2;
	int colno;
	int lineno1=0,lineno2=0;
	int mismatch_recs=0,goodrecs;
	int first_data_line;
	bool mismatch=false,ifdual=false;
	
	FILE* in2=fopen(infile2,"rt");
	if (!in2) {
		printf("Could not open %s for input\n",(char*)infile2);
		return false;
	}
	
	// Read headers and find columns containing index items
	
	if (!readheader(in2,items2,nitem2,nindexitem,indexitem,indexitemno,
		infile2,1,dval2,ifvalues2,true,lineno2)) {
		return false;
	}
	
	FILE* in1=fopen(infile1,"rt");
	if (!in1) {
		printf("Could not open %s for input\n",(char*)infile1);
		return false;
	}
	
	if (!readheader(in1,items1,nitem1,nindexitem,indexitem,indexitemno,
		infile1,0,dval1,ifvalues1,false,lineno1)) {
		return false;
	}
	
	first_data_line=lineno1;

	// Produce list of items for inclusion in output file
	
	if (true) { 
		nitem=0;
		for (i=0;i<nitem1;i++) {
			items[nitem]=items1[i];
			items[nitem].colno[0]=i;
			items[nitem].colno[1]=-1;
			nitem++;
		}
		warned=false;
		for (i=0;i<nitem2;i++) {
			if (items2[i].indexitemno!=-1) {
				for (j=0;j<nitem1;j++) {
					if (items[j].indexitemno==items2[i].indexitemno)
						items[j].colno[1]=i;
				}
			}
			else {
				items[nitem]=items2[i];
				items[nitem].colno[0]=-1;
				items[nitem].colno[1]=i;
				if (nitem==MAXITEM-1) {
					if (!warned) {
						printf("Warning: too many items - ignoring data past column %d in %s\n",
							i,(char*)infile2);
						warned=true;
					}
				}
				else nitem++;
			}
		}
	}
	
	printf("Reading data from %s ...\n",(char*)infile2);
	
	sfmt.printf("%da",nitem2);
	dfmt.printf("%df",nitem2);
	
	// Transfer data from first row (if all numbers)
	
	nrec=0;

	if (ifvalues2) {
		for (i=0;i<nitem;i++) {
			data[nrec].val[i]=dval2[items[i].colno[1]];
		}
		data[nrec].nrec=1;
		nrec++;
	}
		
	while (!feof(in2)) {
		
		// Read next record in file
		
		if (readrecord(in2,rec,nitem2,nitem,items,sfmt,dfmt,nrec,iffast,1,lineno2,infile2)) {

			if (nrec==MAXGRID) {
				printf("Too many records (>%d) in %s\n",MAXGRID,(char*)infile2);
				return false;
			}

			recno=findrec(rec,nrec,items,nitem,0,0);
			if (recno>=0) {
				printf("Records %d and %d contain same index item values in %s\n",
					nrec+1,recno+1,(char*)infile2);
				printf("(second input file must have unique index item values for each record)\n");
				return false;
			}
			
			for (i=0;i<nitem;i++) {
				
				data[nrec].val[i]=rec.val[i];
			
//				if (items[i].indexitemno==-1) {
//					data[nrec].val[i]=rec.val[i];
//				}
//				else {
//				}
			}
			data[nrec].nrec=1;
			nrec++;
			if (!(nrec%100000)) printf("%d ...\n",nrec);
			
		}
	}

	printf("Reading data from %s ...\n",(char*)infile1);
	
	sfmt.printf("%da",nitem1);
	dfmt.printf("%df",nitem1);

	// Transfer data from first row (if all numbers)
	
	nrec1=0;
	lonely_recs=0;
	
	if (ifvalues1) {
		for (i=0;i<nitem1;i++)
			rec.val[i]=dval1[i];

		recno=findrec(rec,nrec,items,nitem,0,0);
		if (recno<0) {
			lonely_recs++;
		}
		else {
			for (i=0;i<nitem1;i++) {
				data[recno].val[i]=dval1[i];
			}
				
			data[recno].nrec++; // to flag that this record has been merged
			nrec1++;
		}
	}

	// Read in first input file
	
	while (!feof(in1)) {
	
		// Read next record in file
		
		if (readrecord(in1,rec,nitem1,nitem,items,sfmt,dfmt,nrec1,iffast,0,lineno1,infile1)) {
		
			recno=findrec(rec,nrec,items,nitem,0,nrec1);
			if (recno<0) {
				lonely_recs++;
			}
			else {
				if (data[recno].nrec>1 && !ifdual) {
					
					printf("More than one record in %s matches record #%d in %s\n",
						(char*)infile1,recno+1,(char*)infile2);
					printf("(may be further dual records)\n");
					ifdual=true;
				}
				
				for (i=0;i<nitem1;i++) {
					data[recno].val[i]=rec.val[i];
				}
				
				data[recno].nrec++; // to flag that this record has been subtracted
				
				nrec1++;
				if (!(nrec1%100000)) printf("%d ...\n",nrec1);
			}
		}
	}
	
	// Now produce output
	
	FILE* out=fopen(outfile,"wt");
	if (!out) {
		printf("Could not open %s for output\n",(char*)outfile);
		return false;
	}
	
	// Print header row
	
	for (i=0;i<nitem;i++) {
		items[i].compute_fmt();
		if (i) fprintf(out,sep);
		fprintf(out,items[i].lfmt,(char*)items[i].label);
	}
	fprintf(out,"\n");
	
	goodrecs=0;
	
	if (ifdual) {
	
		// Input file 1 contains duplicate records (e.g. same lon/lat, different years)
		// Reread and write record by record
		
		rewind(in1);
		lineno1=0;
		for (i=0;i<first_data_line;i++) readfor(in1,"");
		lineno1=first_data_line;
		
		// Transfer data from first row (if all numbers)
		
		if (ifvalues1) {
			for (i=0;i<nitem1;i++)
				rec.val[i]=dval1[i];
	
			recno=findrec(rec,nrec,items,nitem,0,0);
			if (recno>=0) {
				for (i=0;i<nitem1;i++) {
					data[recno].val[i]=dval1[i];
				}
			}
			
			for (j=0;j<nitem;j++) {
				if (j) fprintf(out,sep);
				fprintf(out,items[j].fmt,(double)data[recno].val[j]);
			}
			fprintf(out,"\n");
			
			goodrecs++;
		}
	
		// Read in first input file
		
		while (!feof(in1)) {
		
			// Read next record in file
			
			if (readrecord(in1,rec,nitem1,nitem,items,sfmt,dfmt,nrec1,iffast,0,lineno1,infile1)) {
			
				recno=findrec(rec,nrec,items,nitem,0,nrec1);
				if (recno>=0) {
					
					for (i=0;i<nitem1;i++) {
						data[recno].val[i]=rec.val[i];
					}
					
					for (j=0;j<nitem;j++) {
						if (j) fprintf(out,sep);
						fprintf(out,items[j].fmt,(double)data[recno].val[j]);
					}
					fprintf(out,"\n");

					goodrecs++;
					if (!(goodrecs%100000)) printf("%d ...\n",goodrecs);
				}
			}
		}
	}
	else {
	
		// One-to-one record match between input files
		// Print out data stored in global array
		
		for (i=0;i<nrec;i++) {

			if (data[i].nrec==2) {
				for (j=0;j<nitem;j++) {
					if (j) fprintf(out,sep);
					fprintf(out,items[j].fmt,(double)data[i].val[j]);
				}
				fprintf(out,"\n");
				goodrecs++;
			}
			else {
				mismatch=true;
				mismatch_recs++;
			}
		}
	}

	fclose(in1);
	fclose(in2);
	fclose(out);
	
	printf("\n");
	if (mismatch || lonely_recs) {
		printf("Warning: not all records were common to %s and %s:\n",
			(char*)infile1,(char*)infile2);
		if (mismatch) printf("%d records present in %s but not %s\n",mismatch_recs,
			(char*)infile2,(char*)infile1);
		if (lonely_recs) printf("%d records present in %s but not %s\n",lonely_recs,
			(char*)infile1,(char*)infile2);
	}
	
	printf("\n%d records written to %s\n\n",goodrecs,(char*)outfile);
	
	return true;
}

bool writedata(xtring filename,Item* items,int& nitem,int nrec,char* sep,
	xtring infile1,xtring infile2,int lonely_recs) {
	
	int i,j,mismatch_recs=0,good_recs=0;
	bool mismatch=false;
	
	FILE* out=fopen(filename,"wt");
	if (!out) {
		printf("Could not open %s for output\n",(char*)filename);
		return false;
	}
	
	// Print header row
	
	for (i=0;i<nitem;i++) {
		items[i].compute_fmt();
		if (i) fprintf(out,sep);
		fprintf(out,items[i].lfmt,(char*)items[i].label);
	}
	fprintf(out,"\n");

	// Print data
	
	for (i=0;i<nrec;i++) {

		if (data[i].nrec==2) {
			for (j=0;j<nitem;j++) {
				if (j) fprintf(out,sep);
				fprintf(out,items[j].fmt,(double)data[i].val[j]);
			}
			fprintf(out,"\n");
			good_recs++;
		}
		else {
			mismatch=true;
			mismatch_recs++;
		}
	}
	
	fclose(out);
	
	printf("\n");
	if (mismatch || lonely_recs) {
		printf("Warning: not all records were common to %s and %s:\n",
			(char*)infile1,(char*)infile2);
		if (mismatch) printf("%d records present in %s but not %s\n",mismatch_recs,
			(char*)infile2,(char*)infile1);
		if (lonely_recs) printf("%d records present in %s but not %s\n",lonely_recs,
			(char*)infile1,(char*)infile2);
	}
	
	printf("\n%d records written to %s\n\n",good_recs,(char*)filename);
	
	return true;
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

	fprintf(out,"JOYN\n");
	fprintf(out,"Joins records with shared values of one or more common items in two\n");
	fprintf(out,"plain text input files.\n\n");
	fprintf(out,"Usage: %s <input-file-1> <input-file-2> <options>\n\n",(char*)exe);
	fprintf(out,"Options:\n");
	fprintf(out,"-i <item-name> | <column-number> { <item-name> | <column-number> }\n");
	fprintf(out,"    Index item names or 1-based column numbers. These items are used to\n");
	fprintf(out,"    identify matching records in <input-file-1> and <input-file-2>.\n");
	fprintf(out,"    Each set of index item values must be unique in <input-file-2>.\n");
	fprintf(out,"    <input-file-1> may contain multiple records matching a single record\n");
	fprintf(out,"    in <input-file-2>.\n");
	fprintf(out,"-o <output-file>\n");
	fprintf(out,"    Pathname for output file containing all items in matching records\n");
	fprintf(out,"    from both input files\n");
	fprintf(out,"-tab\n");
	fprintf(out,"    Tab-delimited output\n");
	fprintf(out,"-fast\n");
	fprintf(out,"    Fast mode with tab-delimited output\n");
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

	printf("Usage: %s <input-file-1> <input-file-2> <options>\n",(char*)exe);
	printf("Options: -i <item-name> | <column-number> { <item-name> | <column-number> }\n");
	printf("         -o <output-file>\n");
	printf("         -tab\n");
	printf("         -fast\n");
	printf("         -help\n");

	exit(99);
}

bool processargs(int argc,char* argv[],xtring& infile1,xtring& infile2,xtring& outfile,
	xtring& sep,xtring indexitem[MAXINDEX],int indexitemno[MAXINDEX],int& nindexitem,
	bool& iffast) {

	int i,itemno,ninfile=0;
	xtring arg,item;
	bool slut;
	double dval;
	sep=" ";
	
	// Defaults
	outfile="";
	iffast=false;
	for (i=0;i<MAXINDEX;i++) {
		indexitem[i]="";
		indexitemno[i]=0;
	}
	nindexitem=0;
	
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
			else if (arg=="-i") { // index column label or item number
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
									printf("Option -i: too many items (maximum allowed is %d)\n",MAXITEM);
									return false;
								}
								indexitemno[nindexitem++]=itemno;
							}
						}
						else indexitem[nindexitem++]=item;
						i++;
					}
				}
				if (!nindexitem) {
					printf("Option -i must be followed by label or item number for at least one index item\n");
					return false;
				}
			}
			else if (arg=="-tab") {
				sep="\t";
			}
			else if (arg=="-fast") {
				sep="\t";
				iffast=true;
			}
			else if (arg=="-h" || arg=="-help") printhelp(argv[0]);
			else {
				printf("Invalid option %s\n",(char*)arg);
				return false;
			}
		}
		else {
			if (ninfile==2) {
				printf("Exactly two input files must be specified\n");
				return false;
			}
			else {
				if (!ninfile) infile1=arg;
				else infile2=arg;
				ninfile++;
			}
		}
	}
	
	if (ninfile!=2) {
		printf("File or pathname for exactly two input files must be specified\n");
		return false;
	}
	
	if (outfile=="") {
		
		xtring filepart=infile1;
		stripfilename(filepart);
		
		outfile.printf("%s_joyn.txt",(char*)filepart);
	}
	
	return true;
}


int main(int argc,char* argv[]) {

	xtring infile1,infile2,outfile,header;
	xtring indexitem[MAXINDEX];
	int indexitemno[MAXINDEX],nindexitem;
	bool iffast;
	Item items[MAXITEM];
	int nitem,nrec,lonely_records;
	xtring sep;
	
	if (!processargs(argc,argv,infile1,infile2,outfile,sep,indexitem,
		indexitemno,nindexitem,iffast))
			abort(argv[0]);

	unixtime(header);
	header=(xtring)"[JOYN  "+header+"]\n\n";
	printf("%s",(char*)header);
	
	if (readwritedata(infile1,infile2,outfile,items,nitem,indexitem,indexitemno,nindexitem,
		nrec,iffast,lonely_records,sep)) {

		//writedata(outfile,items,nitem,nrec,sep,infile1,infile2,lonely_records);
	} 
	
	return 0;
}
