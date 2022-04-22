////////////////////////////////////////////////////////////////////////////////////////
// TSLICE
// Postprocessing utility for LPJ-GUESS
// Takes raw ASCII output file with header row from LPJ-GUESS as input file
// Generates output file with one record per grid cell for a single time step or
// averaged over a time slice or all time steps
//
// Written by Ben Smith
// This version dated 2006-04-06
//
// tslice -help for documentation 

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <gutil.h>
#include <vector>
#include <map>

using namespace std;

const int MAXITEM=380;
	// Maximum number of items in a record (row) of an output file
	// (not including longitude,latitude,id and year)

class Record {

public:
	float lon;
	float lat;
	float year;
	int nrec;
	float val[MAXITEM];
	
	Record() {
	
		int i;
		
		nrec=0;
		lon=lat=0.0;
		year=0.0;
		
		for (i=0;i<MAXITEM;i++) val[i]=0.0;
	}
	
	void add_record(Record& rec) {
	
		// Function to add another record to this record
		
		int i;
		
		if (lon!=rec.lon || lat!=rec.lat) {
			printf("add_record: lon or lat mismatch\n");
			exit(99);
		}
		
		for (i=0;i<MAXITEM;i++) val[i]+=rec.val[i];
		nrec+=rec.nrec;
	}
	
	void average() {
	
		// Calculates average over number of added records
		
		int i;
		if (nrec)
			for (i=0;i<MAXITEM;i++) val[i]/=(double)nrec;
	}
};

class Item {

public:
	xtring label,fmt,lfmt;
	bool ifnum;
	bool ifsign;
	int places;
	int digits;
	
	Item() {
		label="";
		ifnum=true;
		ifsign=false;
		places=digits=0;
	}
	
	void compute_fmt() {
	
		int w;
		
		if (digits) w=digits;
		else w=1;
		
		if (places) w+=places+1;
		if (ifsign) w++;
		
		if (label.len()>w) w=label.len();
		
		if (ifnum)
			fmt.printf("%%%d.%df",w,places);
		else fmt.printf("%%%dg",w);
		
		lfmt.printf("%%%ds",w);
	}
};

// Global matrix to store data from files
vector<Record> data;

// A map which maps from coordinates to indexes into the data vector
map<pair<float, float>, int> coords_to_record_pos;

bool scanitem(const char* text,int& places,int& digits,bool& ifsign) {

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
			ifnum=false;
		}
		i++;
		ch=text[i];
	}
		
	return ifnum;
}


bool readheader(FILE*& in,Item* items,int& ncol,
	int& lonitemno,int& latitemno,int& yearitemno,xtring filename,
	double values[MAXITEM],bool& ifvalues,int& lineno) {

	// Reads header row of an LPJ-GUESS output file
	// label = array of header labels
	// ncol  = number of columns (labels)
	// lonitemno = guess at column number (1-based) containing longitude
	// latitemno = guess at column number (1-based) containing latitude
	// yearitemno = guess at column number (1-based) containing year or time step
	// Returns false if too many items in file
	
	xtring line,item;
	int pos,i;
	bool alphabetics=false;
	ncol=0;
	
	while (!ncol && !feof(in)) {
	
		readfor(in,"a#",&line);
		lineno++;
		
		lonitemno=-1;
		latitemno=-1;
		yearitemno=-1;
	
		pos=line.findnotoneof(" \t");
		while (pos!=-1) {
			line=line.mid(pos);
			pos=line.findoneof(" \t");
			if (ncol>MAXITEM+2) {
				printf("Too many columns (>%d) in %s\n",MAXITEM,(char*)filename);
				return false;
			}
			if (pos>0) {
				item=line.left(pos);
				items[ncol].label=item;
				line=line.mid(pos);
				pos=line.findnotoneof(" \t");
			}
			else {
				item=line;
				items[ncol].label=item;
			}
			
			if (item.isnum()) {
				values[ncol]=item.num();
				scanitem(item,items[ncol].places,items[ncol].digits,items[ncol].ifsign);
			}
			else
				alphabetics=true;
			
			ncol++;
			
			if (item.lower()=="year" && yearitemno<0) yearitemno=ncol;
			else if (item.len()>2) {
				if (lonitemno<0)
					if (item.left(3).lower()=="lon") lonitemno=ncol;
				if (latitemno<0)
					if (item.left(3).lower()=="lat") latitemno=ncol;
			}
		}
	}
	
	if (!ncol) {
		printf("%s contains no data\n",(char*)filename);
		return false;
	}
	else if (ncol<3) {
		printf("At least three columns expected in %s\n",(char*)filename);
		return false;
	}
		
	if (lonitemno<0 || latitemno<0) {
		lonitemno=1;
		latitemno=2;
	}
	
	if (yearitemno<0) yearitemno=3;

	if (!alphabetics) {
	
		for (i=0;i<ncol;i++) {
			items[i].label.printf("Column%d",i+1);
		}
		
		ifvalues=true;
	}
	else ifvalues=false;

	return true;
}

bool readrecord(FILE*& in,Record& rec,int nitem,int lonitemno,int latitemno,int yearitemno,
	Item* items,const char* sfmt,const char* dfmt,int nrec,bool iffast,int& lineno,xtring& filename) {

	// Reads one record (row) in output file
	// Returns false on end of file
	// nitem = total number of items including lon, lat, year
	// lonitemno = column number (0-based) containing longitude
	// latitemno = column number (0-based) containing latitude
	// yearitemno = column number (0-based) containing year or time step
	
	static xtring sval[MAXITEM]; // static to avoid reallocating in each call
	double dval[MAXITEM];
	int i,places,digits;
	bool ifsign,searching=true,blank,isnum;

	while (searching) {
		if (nrec<100 || !(nrec%10) || !iffast) {
		
			if (!readfor(in,sfmt,sval)) return false;
			lineno++;
			
			blank=true;
			isnum=true;
			for (i=0;i<nitem;i++) {
				if (sval[i]!="") {
					blank=false;
					if (!sval[i].isnum()) {
						isnum=false;
						i=nitem;
					}
				}
			}
			
			if (blank)
				printf("Line %d of %s is blank - ignoring\n",lineno,(char*)filename);
			else if (!isnum) {
				printf("Line %d of %s contains non-numeric data - ignoring entire line\n",
					lineno,(char*)filename);
			}
			else {
				for (i=0;i<nitem;i++) {
					 if (scanitem(sval[i],places,digits,ifsign)) {
						if (places>items[i].places) items[i].places=places;
						if (digits>items[i].digits) items[i].digits=digits;
						if (ifsign) items[i].ifsign=true;
					}
					else items[i].ifnum=false;
					dval[i]=sval[i].num();
					searching=false;
				}
			}
		}
		else {
			if (!readfor(in,dfmt,dval)) return false;
			lineno++;
			searching=false;
		}
		
		rec.lon=dval[lonitemno];
		rec.lat=dval[latitemno];
		rec.year=dval[yearitemno];
		for (i=0;i<nitem;i++) rec.val[i]=dval[i];
		rec.nrec=1;
	}
	
	return true;
}

bool finditem(xtring item,int& itemno,xtring infile,Item* items,int nitem) {

	int i;
	
	if (itemno) {
		// taking data from specified column number - no header assumed
		
		itemno--; // convert to 0-based
	}
	else {
		itemno=-1;
		for (i=0;i<nitem;i++) {
			if (items[i].label==item) {
				itemno=i;
				i=nitem;
			}
		}
		if (itemno==-1) {
		
			// Not found - try case insensitive comparison
			
			itemno=-1;
			for (i=0;i<nitem;i++) {
				if (items[i].label.lower()==item.lower()) {
					itemno=i;
					i=nitem;
				}
			}
			
			if (itemno==-1) {
				printf("Item %s not found in %s\n",(char*)item,(char*)infile);
				return false;
			}
			else {
				printf("Item %s not found in %s\n",(char*)item,(char*)infile);
				printf("Choosing %s instead\n",(char*)items[itemno].label);
			}
		}
	}
	
	return true;
}

int findrec(float lon,float lat) {

	// Returns record number in global data corresponding to given longitude and latitude
	// -1 if not found
	
	if (data.empty()) {
		return -1;
	}

	// Start by checking the latest record, this should be a match most of the
	// time if input is sorted by grid cell.
	const Record& latest = data.back();

	if (latest.lon == lon && latest.lat == lat) {
		return data.size()-1;
	}

	// If no match, search the map
	map<pair<float,float>, int>::const_iterator itr = 
		coords_to_record_pos.find(make_pair(lon, lat));

	if (itr != coords_to_record_pos.end()) {
		return (*itr).second;
	}
	else {
		return -1;
	}
}

bool readdata(xtring filename,float fromyear,float toyear,bool iffrom,bool ifto,
	Item* items,int& nitem,int& lonitemno,int& latitemno,int& yearitemno,
	xtring lonitem,xtring latitem,xtring yearitem,bool iffast) {

	int recno,i;
	int autolonitem,autolatitem,autoyearitem;
	double dval[MAXITEM];
	bool ifvalues;
	int lineno=0;
	xtring sfmt,dfmt;
	Record rec;
	
	FILE* in=fopen(filename,"rt");
	if (!in) {
		printf("Could not open %s for input\n",(char*)filename);
		return false;
	}

	// Read header and find columns containing lon, lat, year
	if (!readheader(in,items,nitem,autolonitem,autolatitem,autoyearitem,
		filename,dval,ifvalues,lineno)) {
		return false;
	}
	
	sfmt.printf("%da",nitem);
	dfmt.printf("%df",nitem);
	
	if (lonitem=="" && lonitemno==0) lonitemno=autolonitem;
	if (latitem=="" && latitemno==0) latitemno=autolatitem;
	if (yearitem=="" && yearitemno==0) yearitemno=autoyearitem;
	
	if (!finditem(lonitem,lonitemno,filename,items,nitem)) return false;
	if (!finditem(latitem,latitemno,filename,items,nitem)) return false;
	if (!finditem(yearitem,yearitemno,filename,items,nitem)) return false;
	
	if (lonitemno==latitemno || lonitemno==yearitemno || latitemno==yearitemno) {
		printf("Error: longitude, latitude and year expected in separate columns %d %d %d\n",
			lonitemno,latitemno,yearitemno);
		return false;
	}
	
	if (lonitem=="") lonitem="Lon";
	if (latitem=="") latitem="Lat";
	items[lonitemno].label=lonitem;
	items[latitemno].label=latitem;
	
	if (iffrom && ifto) {
		if (fromyear==toyear)
			printf("Extracting data for time step %g\n",(double)fromyear);
		else printf("Averaging over time slice from %g to %g\n",(double)fromyear,(double)toyear);
	}
	else if (iffrom) {
		printf("Averaging over data from time step %g onwards\n",(double)fromyear);
	}
	else if (ifto) {
		printf("Averaging over data up to time step %g\n",(double)toyear);
	}
	else printf("Averaging over data from all time steps\n");

	printf("Reading data from %s ...\n",(char*)filename);
	
	// Transfer data from first row (if all numbers)
	
	if (ifvalues) {
		if ((dval[yearitemno]>=fromyear || !iffrom) && (dval[yearitemno]<=toyear || !ifto)) {
			Record rec;
			rec.lon=dval[lonitemno];
			rec.lat=dval[latitemno];
			rec.nrec=1;
			for (i=0;i<nitem;i++) rec.val[i]=dval[i];
			data.push_back(rec);
			coords_to_record_pos[make_pair(rec.lon, rec.lat)] = data.size()-1;
		}
	}
		
	while (!feof(in)) {
		
		// Read next record in file
		
		if (readrecord(in,rec,nitem,lonitemno,latitemno,yearitemno,items,
							sfmt,dfmt,data.size(),iffast,lineno,filename)) {
		
			if ((rec.year>=fromyear || !iffrom) && (rec.year<=toyear || !ifto)) {
			
				recno=findrec(rec.lon,rec.lat);
				if (recno<0) {
					Record new_rec;
					new_rec.lon=rec.lon;
					new_rec.lat=rec.lat;
					recno=data.size();
					data.push_back(new_rec);
					coords_to_record_pos[make_pair(new_rec.lon, new_rec.lat)] = data.size()-1;
					if (!(data.size()%5000)) printf("%d ...\n",data.size());
				}
				
				data[recno].add_record(rec);
			}
		}
	}
	
	// Now calculate averages for timeslice
	
	for (i = 0; i < data.size(); i++) {
		 data[i].average();
	}
	
	fclose(in);

	return true;
}

bool writedata(xtring filename,Item* items,int& nitem,int lonitemno,int latitemno,
	int yearitemno,char* sep) {
	
	int i,j;
	
	FILE* out=fopen(filename,"wt");
	if (!out) {
		printf("Could not open %s for output\n",(char*)filename);
		return false;
	}
	
	// Print header row
	
	items[lonitemno].compute_fmt();
	fprintf(out,items[lonitemno].lfmt,(char*)items[lonitemno].label);
	fprintf(out,sep);
	items[latitemno].compute_fmt();
	fprintf(out,items[latitemno].lfmt,(char*)items[latitemno].label);
	
	for (i=0;i<nitem;i++) {
		if (i!=lonitemno && i!=latitemno && i!=yearitemno) {
			items[i].compute_fmt();
			fprintf(out,sep);
			fprintf(out,items[i].lfmt,(char*)items[i].label);
		}
	}
	fprintf(out,"\n");

	// Print data

	for (i = 0; i < data.size(); i++) {
	
		fprintf(out,items[lonitemno].fmt,(double)data[i].lon);
		fprintf(out,sep);
		fprintf(out,items[latitemno].fmt,(double)data[i].lat);
		
		for (j=0;j<nitem;j++) {
			if (j!=lonitemno && j!=latitemno && j!=yearitemno) {
				fprintf(out,sep);
				fprintf(out,items[j].fmt,(double)data[i].val[j]);
			}
		}
		fprintf(out,"\n");
	}
	
	fclose(out);
	
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

	fprintf(out,"Usage: %s <input-file> <options>\n\n",(char*)exe);
	fprintf(out,"Options:\n\n");
	fprintf(out,"-o <output-file>\n");
	fprintf(out,"    Pathname for output file\n");
	fprintf(out,"-f <from-year>\n");
	fprintf(out,"    Lower bound year or time step for time slice to average over\n");
	fprintf(out,"-t <to-year>\n");
	fprintf(out,"    Upper bound year or time step for time slice to average over\n");
	fprintf(out,"-lon <item-name> | <column-number>\n");
	fprintf(out,"    Item name or 1-based column number for longitude data\n");
	fprintf(out,"-lat <item-name> | <column-number>\n");
	fprintf(out,"    Item name or 1-based column number for latitude data\n");
	fprintf(out,"-y <item-name> | <column-number>\n");
	fprintf(out,"    Item name or 1-based column number for year or time step data\n");
	fprintf(out,"-tab\n");
	fprintf(out,"    Tab-delimited output\n");
	fprintf(out,"-fast\n");
	fprintf(out,"    Fast mode with tab-delimited output\n");
	fprintf(out,"-help\n");
	fprintf(out,"   Displays this help message\n");
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
	printf("         -f <from-year>\n");
	printf("         -t <to-year>\n");
	printf("         -lon <item-name> | <column-number>\n");
	printf("         -lat <item-name> | <column-number>\n");
	printf("         -y <item-name> | <column-number>\n");
	printf("         -tab\n");
	printf("         -fast\n");
	printf("         -help\n");

	exit(99);
}

bool processargs(int argc,char* argv[],xtring& infile,xtring& outfile,
	xtring& lonitem,xtring& latitem,xtring& yearitem,
	int& lonitemno,int& latitemno,int& yearitemno,
	float& fromyear,float& toyear,bool& iffrom,bool& ifto,xtring& sep,
	bool& iffast) {

	int i;
	xtring arg;
	bool haveinfile=false;
	iffrom=false,ifto=false;
	iffast=false;
	double dval;
	sep=" ";
	
	// Defaults
	outfile="";
	lonitem=latitem=yearitem="";
	lonitemno=latitemno=yearitemno=0;
	toyear=fromyear=0;
	
	
	if (argc<2) {
		printf("Input file name or path must be specified\n");
		return false;
	}
	
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
			else if (arg=="-lon") { // longitude column label or item number 
				if (argc>=i+2) {
					lonitem=argv[i+1];
					if (lonitem.isnum()) {
						dval=lonitem.num();
						if (dval>=1.0 && int(dval)==dval) { // seems to be an item number
							lonitemno=dval;
							if (lonitemno>MAXITEM) {
								printf("Option -lon: too many items (maximum allowed is %d)\n",MAXITEM);
								return false;
							}
							lonitem="";
						}
					}
				}
				else {
					printf("Option -lon must be followed by label or column number\n");
					return false;
				}
				i+=1;
			}
			else if (arg=="-lat") { // latitude column label or item number 
				if (argc>=i+2) {
					latitem=argv[i+1];
					if (latitem.isnum()) {
						dval=latitem.num();
						if (dval>=1.0 && int(dval)==dval) { // seems to be an item number
							latitemno=dval;
							if (latitemno>MAXITEM) {
								printf("Option -lat: too many items (maximum allowed is %d)\n",MAXITEM);
								return false;
							}
							latitem="";
						}
					}
				}
				else {
					printf("Option -lat must be followed by label or column number\n");
					return false;
				}
				i+=1;
			}
			else if (arg=="-y" || arg=="-year") { // year column label or item number 
				if (argc>=i+2) {
					yearitem=argv[i+1];
					if (yearitem.isnum()) {
						dval=yearitem.num();
						if (dval>=1.0 && int(dval)==dval) { // seems to be an item number
							yearitemno=dval;
							if (yearitemno>MAXITEM) {
								printf("Option -y: too many items (maximum allowed is %d)\n",MAXITEM);
								return false;
							}
							yearitem="";
						}
					}
				}
				else {
					printf("Option -y must be followed by label or column number\n");
					return false;
				}
				i+=1;
			}
			else if (arg=="-f" || arg=="-from") { // "from" year
				if (argc>=i+2) {
					arg=argv[i+1];
					if (!arg.isnum()) {
						printf("Option -f must be followed by a number\n");
						return false;
					}
					fromyear=arg.num();
					iffrom=true;
				}
				else {
					printf("Option -f must be followed by a number\n");
					return false;
				}
				i+=1;
			}
			else if (arg=="-t" || arg=="-to") { // "to" year
				if (argc>=i+2) {
					arg=argv[i+1];
					if (!arg.isnum()) {
						printf("Option -t must be followed by a number\n");
						return false;
					}
					toyear=arg.num();
					ifto=true;
				}
				else {
					printf("Option -t must be followed by a number\n");
					return false;
				}
				i+=1;
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
			if (haveinfile) {
				printf("Only one input file may be specified\n");
				return false;
			}
			else {
				infile=arg;
				haveinfile=true;
			}
		}
	}
	
	if (!haveinfile) {
		printf("Input file name or path must be specified\n");
		return false;
	}
	
	if (iffrom && ifto && toyear<fromyear) {
		printf("'To' year must be same as or later than 'from' year\n");
		return false;
	}
	
	if (outfile=="") {
		
		xtring filepart=infile;
		stripfilename(filepart);
		
		if (ifto && iffrom) {
			if (fromyear==toyear)
				outfile.printf("%s_%g.txt",(char*)filepart,(double)fromyear);
			else
				outfile.printf("%s_%g-%g.txt",(char*)filepart,(double)fromyear,(double)toyear);
		}
		else if (ifto) {
			outfile.printf("%s_-%g.txt",(char*)filepart,(double)toyear);
		}
		else if (iffrom) {
			outfile.printf("%s_%g-.txt",(char*)filepart,(double)fromyear);
		}
		else {
			outfile.printf("%s_mean.txt",(char*)filepart);
		}
	}
	
	return true;
}


int main(int argc,char* argv[]) {

	xtring infile,outfile,lonitem,latitem,yearitem,header;
	int lonitemno,latitemno,yearitemno;
	float fromyear,toyear;
	bool iffrom,ifto,iffast;
	Item items[MAXITEM];
	int nitem,nrec;
	xtring sep;
	
	if (!processargs(argc,argv,infile,outfile,lonitem,latitem,yearitem,
		lonitemno,latitemno,yearitemno,fromyear,toyear,iffrom,ifto,sep,iffast))
			abort(argv[0]);

	unixtime(header);
	header=(xtring)"[TSLICE  "+header+"]\n\n";
	printf("%s",(char*)header);

	if (readdata(infile,fromyear,toyear,iffrom,ifto,items,nitem,
		lonitemno,latitemno,yearitemno,lonitem,latitem,yearitem,iffast)) {
	
		if (writedata(outfile,items,nitem,lonitemno,latitemno,yearitemno,sep)) {
		
			printf("\n%d records written to %s\n\n",data.size(),(char*)outfile);
		}
	} 
	
	return 0;
}
