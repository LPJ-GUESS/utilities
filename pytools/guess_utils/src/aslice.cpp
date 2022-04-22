////////////////////////////////////////////////////////////////////////////////////////
// ASLICE
// Postprocessing utility for LPJ-GUESS
// Takes raw ASCII output file with header row from LPJ-GUESS as input file
// Generates output file with one record per year or timestep containing weighted
// average over a geographic window
//
// Written by Ben Smith
// This version dated 2006-05-05
//
// aslice -help for documentation 

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <gutil.h>
#include <math.h>
#include <vector>

using namespace std;

const int MAXITEM=380;
	// Maximum number of items in a record (row) of an output file
	// (not including longitude,latitude,id and year)
const int MAXYEAR=10000;
	// Maximum number of timesteps (need not be consecutive) in time series

class Record {

public:
	float lon;
	float lat;
	float year;
	int nrec;
	float val[MAXITEM];
	double area;
	
	Record() {
	
		int i;
		
		nrec=0;
		lon=lat=0.0;
		year=0.0;
		area=0.0;
		
		for (i=0;i<MAXITEM;i++) val[i]=0.0;
	}
	
	void add_record(Record& rec) {
	
		// Function to add another record to this record
		// Pixel area (for weighting) should be set in rec
		
		int i;
		
		if (year!=rec.year) {
			printf("add_record: timestep mismatch\n");
			exit(99);
		}

		for (i=0;i<MAXITEM;i++) val[i]+=rec.val[i]*rec.area;
		nrec+=rec.nrec;
		area+=rec.area;
	}
	
	void average() {
	
		// Calculates area-based average over number of added records
		
		int i;
		if (area)
			for (i=0;i<MAXITEM;i++) {
				//printf("%d: value=%g area=%g average=%g\n",i,val[i],area,val[i]/area);
				val[i]/=area;
			}
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

struct Point {

	Point(float xarg = 0, float yarg = 0) : x(xarg), y(yarg) {}

	float x;
	float y;
};

// Global matrix to store data from files
Record data[MAXYEAR];

// Timestep as specified in input file
float guessyear[MAXYEAR];

// Number of unique timesteps in input file
int nyear;

double pixelsize(double longpos,double latpos,double longsize,double latsize,int postype) {

	// Returns area in square km of a pixel of a given size at a given point
	// on the world.  The formula applied is the surface area of a segment of
	// a hemisphere of radius r from the equator to a parallel (circular)
	// plane h vertical units towards the pole: S=2*pi*r*h

	// longpos   longitude position (see postype)
	// latpos    latitude position (see postype)
	// longsize  longitude range in degrees
	// latsize   latitude range in degrees
	// postype   declares which part of the pixel longpos and latpos
	//           refer to:
	//           0 = centre
	//           1 = NW corner
	//           2 = NE corner
	//           3 = SW corner
	//           4 = SE corner
      
      double pi,r,h1,h2,lattop,latbot,s;
      
      pi=3.1415926536;
      r=6367.425;   // mean radius of the earth

      lattop=latpos;
      if (postype==0) lattop=latpos+latsize*0.5;
      if (postype==3 || postype==4) lattop=latpos+latsize;
      if (lattop<0.0) lattop=-lattop+latsize;
      latbot=lattop-latsize;
      h1=r*sin(lattop*pi/180.0);
      h2=r*sin(latbot*pi/180.0);
      s=2.0*pi*r*(h1-h2);  //for this latitude band
	 
      return s*longsize/360.0;  //for this pixel
}

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
	
		pos=line.findnotoneof(" \t\r");
		while (pos!=-1) {
			line=line.mid(pos);
			pos=line.findoneof(" \t\r");
			if (ncol>MAXITEM+2) {
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
	Item* items,const char* sfmt,const char* dfmt,bool iffast,int& lineno,xtring& filename,bool ifyear) {

	// Reads one record (row) in output file
	// Returns false on end of file
	// nitem = total number of items including lon, lat, year
	// lonitemno = column number (0-based) containing longitude
	// latitemno = column number (0-based) containing latitude
	// yearitemno = column number (0-based) containing year or time step
	
	static xtring sval[MAXITEM]; // static to avoid reallocating in each call
	xtring line,whole_line;
	double dval[MAXITEM];
	int i,places,digits,pos;
	bool ifsign,searching=true,blank,isnum;

	while (searching) {
		if (!iffast) {
		
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
		if (ifyear) rec.year=dval[yearitemno];
		else rec.year=1;
		for (i=0;i<nitem;i++) rec.val[i]=dval[i];
		rec.nrec=1;
	}
	
	return true;
}

int year_number(float& year) {

	// Returns index of specified guess year in array guessyear
	// -1 if that year doesn't exist
	
	int y;
	
	for (y=0;y<nyear;y++)
		if (guessyear[y]==year) return y;
		
	return -1;
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

void add_point(vector<Point>& pixdata, Point newpoint) {

	int i;
	
	for (i = pixdata.size()-1; i >= 0; i--) {
		if (pixdata[i].x==newpoint.x && pixdata[i].y==newpoint.y) return;
	}
	
	pixdata.push_back(newpoint);
}

bool estimate_pixsize(const vector<Point>& pixdata,double& pixx,double& pixy,double& pixdx,double& pixdy,
	bool setpix,bool setoffset) {

	const int NSAMPLE=10; // Number of places in data matrix to search for nearest neighbours
	int i,j,targ;
	double minx,miny,diff;
	bool firstx=true,firsty=true;
	
	for (i=0;i<NSAMPLE;i++) {
		 targ=(double)pixdata.size()/(double)NSAMPLE*(double)i;
		 for (j=0;j<pixdata.size();j++) {
			if (firstx) {
				if (pixdata[targ].x!=pixdata[j].x) {
					minx=fabs(pixdata[targ].x-pixdata[j].x);
					firstx=false;
				}
			}
			else {
				diff=fabs(pixdata[targ].x-pixdata[j].x);
				if (diff && diff<minx) minx=diff;
			}
			if (firsty) {
				if (pixdata[targ].y!=pixdata[j].y) {
					miny=fabs(pixdata[targ].y-pixdata[j].y);
					firsty=false;
				}
			}
			else {
				diff=fabs(pixdata[targ].y-pixdata[j].y);
				if (diff && diff<miny) miny=diff;
			}
		}
	}
	
	if (firstx || firsty) return false;
	
	if (setpix) {
		pixx=minx;
		pixy=miny;
	}
	
	if (setoffset) {
		pixdx=minx*0.5;
		pixdy=miny*0.5;
	}
	
	return true;
}

bool preread(FILE*& in,double dval[MAXITEM],bool ifvalues,int nitem,int lonitemno,
	int latitemno,int yearitemno,Item* items,int& lineno,xtring& sfmt,xtring& dfmt,
	xtring filename,double& pixx,double& pixy,double& pixdx,double& pixdy,
	bool havepixsize,bool havepixoffset,bool ifyear) {

	Record rec;

	vector<Point> pixdata;
	Point thispix;

	if (ifvalues) {
		pixdata.push_back(Point(dval[lonitemno], dval[latitemno]));
	}
		
	while (!feof(in)) {
		
		// Read next record in file
		
		if (readrecord(in,rec,nitem,lonitemno,latitemno,yearitemno,items,
			sfmt,dfmt,true,lineno,filename,ifyear)) {
			
			thispix.x=rec.val[lonitemno];
			thispix.y=rec.val[latitemno];

			add_point(pixdata,thispix);
		}
	}
	
	printf("\n");
	
	if (!estimate_pixsize(pixdata,pixx,pixy,pixdx,pixdy,!havepixsize,!havepixoffset)) {
		if (!havepixsize) {
			printf("Assuming default pixel size (0.5,0.5)\n");
			printf("-pixsize to change\n");
			pixx=0.5;
			pixy=0.5;
		}
		if (!havepixoffset) {
			printf("Assuming default pixel offset (0.25,0.25)\n");
			printf("-pixoffset to change\n");
			pixdx=0.25;
			pixdy=0.25;
		}
	}
	else {
		if (!havepixsize) {
			printf("Pixel size seems to be (%g,%g)\n",pixx,pixy);
			printf("-pixsize to change\n");
		}
		if (!havepixoffset) {
			printf("Guessing pixel offset at (%g,%g)\n",pixdx,pixdy);
			printf("-pixoffset to change\n");
		}
	}
	
	return true;
}


bool readdata(xtring filename,float north,float south,float east,float west,
	Item* items,int& nitem,int& lonitemno,int& latitemno,int& yearitemno,int& witemno,
	xtring lonitem,xtring latitem,xtring yearitem,xtring witem,bool iffast,
	double pixx,double pixy,double pixdx,double pixdy,
	bool havepixsize,bool havepixoffset,bool ifweight,bool ifyear) {

	int recno,i,index;
	int autolonitem,autolatitem,autoyearitem;
	double dval[MAXITEM];
	bool ifvalues,ifwitem;
	int lineno=0,lineno_bak;
	xtring sfmt,dfmt;
	Record rec;
	
	nyear=0;
	
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
	if (witem=="" && witemno==0) {
		ifwitem=false;
		witemno=-1;
	}
	else ifwitem=true;
	
	if (!finditem(lonitem,lonitemno,filename,items,nitem)) return false;
	if (!finditem(latitem,latitemno,filename,items,nitem)) return false;
	if (ifyear) {
		if (!finditem(yearitem,yearitemno,filename,items,nitem)) return false;
	}
	if (ifwitem) {
		if (!finditem(witem,witemno,filename,items,nitem)) return false;
	}
	
	if (lonitemno==latitemno || ifyear && (lonitemno==yearitemno || latitemno==yearitemno)) {
		printf("Error: longitude, latitude and timestep expected in separate columns %d %d %d\n",
			lonitemno,latitemno,yearitemno);
		return false;
	}
	
	//if (yearitem=="") yearitem="Year";
	//items[yearitemno].label=yearitem;
	
	if (north==south && east==west)
		printf("Extracting data for (%g,%g)\n",east,north);
	else if (north>=90.0 && south<=-90.0 && east>=180.0 && west<=-180.0)
		printf("Extracting data for all grid cell(s)\n");
	else {
		printf("Extracting data for window bounded by ");
		if (west<0.0) printf("%g W",fabs(west));
		else printf("%g E",west);
		if (south<0.0) printf(", %g S",fabs(south));
		else printf(", %g N",south);
		if (east<0.0) printf(", %g W",fabs(east));
		else printf(", %g E",east);
		if (north<0.0) printf(", %g S\n",fabs(north));
		else printf(", %g N\n",north);
	}
	
	// Preread if necessary to determine pixel size and offset
	
	if ((!havepixsize || !havepixoffset) && ifweight) {
	
		lineno_bak=lineno;
		
		if (!preread(in,dval,ifvalues,nitem,lonitemno,latitemno,yearitemno,items,lineno,
			sfmt,dfmt,filename,pixx,pixy,pixdx,pixdy,havepixsize,havepixoffset,ifyear))
				return false;
		
		rewind(in);
		for (i=0;i<lineno_bak;i++) readfor(in,"");
		lineno=lineno_bak;
	}
	
	if (ifwitem && ifweight) {
		printf("\nComputing weighted average\n");
		printf("Weighting by values in column %d (\"%s\") multiplied by pixel area\n",
			witemno+1,(char*)items[witemno].label);
	}
	else if (ifwitem) {
		printf("\nComputing weighted average\n");
		printf("Weighting by values in column %d (\"%s\")\n",
			witemno+1,(char*)items[witemno].label);
	}
	else if (ifweight) {
		printf("\nComputing weighted average\n");
		printf("Weighting by pixel area\n");
	}
	else {
		printf("\nComputing simple average (no weighting)\n");
	}

	printf("\nReading data from %s ...\n",(char*)filename);
	
	// Transfer data from first row (if all numbers)
	
	if (ifvalues) {
		if (dval[lonitemno]>=west && dval[lonitemno]<=east && dval[latitemno]>=south &&
			dval[latitemno]<=north) {
			if (ifyear) data[nyear].year=dval[yearitemno];
			else data[nyear].year=1;
			if (ifwitem) data[nyear].area=dval[witemno];
			else data[nyear].area=1.0;
			if (ifweight) data[nyear].area*=
				pixelsize(dval[lonitemno]+pixdx,dval[latitemno]+pixdy,pixx,pixy,0);
			data[nyear].nrec=1;
			for (i=0;i<nitem;i++) data[nyear].val[i]=dval[i];
			guessyear[nyear]=data[nyear].year;
			nyear++;
		}
	}
		
	while (!feof(in)) {
		
		// Read next record in file
		
		if (readrecord(in,rec,nitem,lonitemno,latitemno,yearitemno,items,
			sfmt,dfmt,iffast,lineno,filename,ifyear)) {
		
			if (rec.lon>=west && rec.lon<=east && rec.lat>=south && rec.lat<=north) {
			
				index=year_number(rec.year);
				if (index==-1) {
				
					if (nyear==MAXYEAR) {
						printf("Too many time steps (>%d) in %s\n",
							MAXYEAR,(char*)filename);
						return false;
					}
				
					data[nyear].year=rec.year;
					index=nyear;
					guessyear[nyear]=rec.year;
					nyear++;
				}
				
				if (ifwitem) rec.area=rec.val[witemno];
				else rec.area=1.0;
				if (ifweight) rec.area*=pixelsize(rec.val[lonitemno]+pixdx,rec.val[latitemno]+pixdy,
						pixx,pixy,0);
				data[index].add_record(rec);
			}
		}
	}
	
	// Now calculate averages for timeslice
	
	for (i=0;i<nyear;i++) {
		data[i].average();
		if (!i) printf("\nWeight for %g is %g\n",data[i].year,data[i].area);
	}
	
	fclose(in);
	
	return true;
}

bool writedata(xtring filename,Item* items,int& nitem,int lonitemno,int latitemno,
	int yearitemno,int witemno,int nrec,char* sep,bool ifyear,bool ifsum) {
	
	int i,j;
	bool first;
	
	FILE* out=fopen(filename,"wt");
	if (!out) {
		printf("Could not open %s for output\n",(char*)filename);
		return false;
	}
	
	// Print header row
	
	first=true;
	if (ifyear) {
		items[yearitemno].compute_fmt();
		fprintf(out,items[yearitemno].lfmt,(char*)items[yearitemno].label);
		first=false;
	}

	for (i=0;i<nitem;i++) {
		if (i!=lonitemno && i!=latitemno && (!ifyear || i!=yearitemno) && i!=witemno) {
			
			// if we're summing, print all columns with exponent
			if (ifsum) {
				items[i].ifnum = false;
				items[i].digits += 4; // length of "e+99"
			}

			items[i].compute_fmt();
			if (!first) fprintf(out,sep);
			fprintf(out,items[i].lfmt,(char*)items[i].label);
			first=false;
		}
	}
	fprintf(out,"\n");

	// Print data

	for (i=0;i<nrec;i++) {

		first=true;	
		if (ifyear) {
			fprintf(out,items[yearitemno].fmt,(double)data[i].year);
			first=false;
		}
		
		for (j=0;j<nitem;j++) {
			if (j!=lonitemno && j!=latitemno && (!ifyear || j!=yearitemno) && j!=witemno) {
				if (!first) fprintf(out,sep);
				if(ifsum){
				  fprintf(out,items[j].fmt,(double)data[i].val[j]*(double)data[i].area);
				}
				else{
				  fprintf(out,items[j].fmt,(double)data[i].val[j]);
				}
				first=false;
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

	fprintf(out,"ASLICE\n");
	fprintf(out,"Extracts weighted-average values for a geographic area from a plain\n");
	fprintf(out,"text input file. Weights may be computed from grid cell dimensions,\n");
	fprintf(out,"or read from an item in the file, or a combination of both.\n\n");
	fprintf(out,"Usage: %s <input-file> <options>\n\n",(char*)exe);
	fprintf(out,"Options:\n\n");
	fprintf(out,"-o <output-file>\n");
	fprintf(out,"    Pathname for output file\n");
	fprintf(out,"-lon <item-name> | <column-number>\n");
	fprintf(out,"    Item name or 1-based column number for longitude data\n");
	fprintf(out,"-lat <item-name> | <column-number>\n");
	fprintf(out,"    Item name or 1-based column number for latitude data\n");
	fprintf(out,"-y <item-name> | <column-number>\n");
	fprintf(out,"    Item name or 1-based column number for year or time step data\n");
	fprintf(out,"-w <item-name> | <column-number>\n");
	fprintf(out,"    Item name or 1-based column number for (optional) weights for\n");
	fprintf(out,"    computing weighted averages over grid cells\n");
	fprintf(out,"    Multiplied by pixel area unless -eq option also specified\n");
	fprintf(out,"-n\n");
	fprintf(out,"    Input file does not contain year or time step data\n");
	fprintf(out,"-eq\n");
	fprintf(out,"    Suppresses weighting by pixel area\n");
	fprintf(out,"-pixsize <x> <y>\n");
	fprintf(out,"    Pixel area in degress of longitude (x) and latitude (y)\n");
	fprintf(out,"    Required for area weighting, ignored if -eq option also specified\n");
	fprintf(out,"-pixoffset <dx> <dy>\n");
	fprintf(out,"    Offset in degrees of centre of pixel from coordinate specified in data\n");
	fprintf(out,"-x <lon> <lat> | <west> <south> <east> <north>\n");
	fprintf(out,"    Coordinate in degrees of a grid cell to extract or\n");
	fprintf(out,"    bounds in degrees of a window to extract\n");
	fprintf(out,"    Default extracts data for all grid cell(s)\n");
	fprintf(out,"-tab\n");
	fprintf(out,"    Tab-delimited output\n");
	fprintf(out,"-sum\n");
	fprintf(out,"    Provide areal sum of values instead of average\n");
	fprintf(out,"    Requires data expressed per m2\n");
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
	printf("         -lon <item-name> | <column-number>\n");
	printf("         -lat <item-name> | <column-number>\n");
	printf("         -y <item-name> | <column-number>\n");
	printf("         -w <item-name> | <column-number>\n");
	printf("         -n\n");
	printf("         -eq\n");
	printf("         -pixsize <x> <y>\n");
	printf("         -pixoffset <dx> <dy>\n");
	printf("         -x <lon> <lat> | <west> <south> <east> <north>\n");
	printf("         -tab\n");
	printf("         -sum\n");
	printf("         -help\n");
	
	exit(99);
}

bool processargs(int argc,char* argv[],xtring& infile,xtring& outfile,
	xtring& lonitem,xtring& latitem,xtring& yearitem,xtring& witem,
	int& lonitemno,int& latitemno,int& yearitemno,int& witemno,
	double& pixx,double& pixy,double& pixdx,double& pixdy,
	bool& havepixsize,bool& havepixoffset,
	double& north,double& south,double& east,double& west,
	float& fromyear,float& toyear,bool& iffrom,bool& ifto,
	xtring& sep,bool& iffast,bool& ifweight,bool& ifyear,bool& ifsum) {

	int i,j,nval;
	xtring arg,thisarg;
	bool haveinfile=false,slut,havewindow=false;
	iffrom=false,ifto=false;
	iffast=false;
	ifyear=true;
	ifsum=false;
	double dval;
	sep=" ";
	havepixsize=havepixoffset=false;
	
	// Defaults
	outfile="";
	lonitem=latitem=yearitem=witem="";
	lonitemno=latitemno=yearitemno=witemno=0;
	toyear=fromyear=0;
	ifweight=true;
	
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
			else if (arg=="-y" || arg=="-year") { // year/timestep column label or item number 
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
			else if (arg=="-w" || arg=="-weight") { // weight column label or item number
				if (argc>=i+2) {
					witem=argv[i+1];
					if (witem.isnum()) {
						dval=witem.num();
						if (dval>=1.0 && int(dval)==dval) { // seems to be an item number
							witemno=dval;
							if (witemno>MAXITEM) {
								printf("Option -w: too many items (maximum allowed is %d)\n",MAXITEM);
								return false;
							}
							witem="";
						}
					}
				}
				else {
					printf("Option -w must be followed by label or column number\n");
					return false;
				}
				i+=1;
			}
			else if (arg=="-pixsize") { // pixel dimensions
				if (argc>=i+3) {
					arg=argv[i+1];
					if (!arg.isnum()) {
						printf("Option -pixsize <x> <y>: <x> must be a number\n");
						return false;
					}
					pixx=arg.num();
					if (pixx<=0.0 || pixx>180.0) {
						printf("Option -pixsize <x> <y>: expecting a number in positive degrees\n");
						return false;
					}
					arg=argv[i+2];
					if (!arg.isnum()) {
						printf("Option -pixsize <x> <y>: <y> must be a number\n");
						return false;
					}
					pixy=arg.num();
					if (pixy<=0.0 || pixy>90.0) {
						printf("Option -pixsize <x> <y>: expecting a number in positive degrees\n");
						return false;
					}
				}
				else {
					printf("Option -pixsize must be followed by <x> <y>\n");
					return false;
				}
				i+=2;
				havepixsize=true;
			}
			else if (arg=="-pixoffset") { // pixel offset from reference point
				if (argc>=i+3) {
					arg=argv[i+1];
					if (!arg.isnum()) {
						printf("Option -pixoffset <dx> <dy>: <dx> must be a number\n");
						return false;
					}
					pixdx=arg.num();
					arg=argv[i+2];
					if (!arg.isnum()) {
						printf("Option -pixoffset <dx> <dy>: <dy> must be a number\n");
						return false;
					}
					pixdy=arg.num();
				}
				else {
					printf("Option -pixoffset must be followed by <dx> <dy>\n");
					return false;
				}
				i+=2;
				havepixoffset=true;
			}
			else if (arg=="-x") { // Window or pixel coordinate to extract
				j=i+1;
				slut=false;
				while (j<argc && !slut) {
					thisarg=argv[j];
					if (thisarg.isnum()) j++;
					else slut=true;
				}
				nval=j-i-1;
				if (nval==2) {
					arg=argv[i+1];
					east=west=arg.num();
					if (east<-180.0 || east>180.0)
						printf("Option -x: <lon> must be in range -180 to 180\n");
					arg=argv[i+2];
					north=south=arg.num();
					if (north<-90.0 || north>90.0)
						printf("Option -x: <lat> must be in range -90 to 90\n");
					i+=2;
				}
				else if (nval==4) {
					arg=argv[i+1];
					west=arg.num();
					if (west<-180.0 || west>180.0)
						printf("Option -x: <west> must be in range -180 to 180\n");
					arg=argv[i+2];
					south=arg.num();
					if (south<-90.0 || south>90.0)
						printf("Option -x: <south> must be in range -90 to 90\n");
					arg=argv[i+3];
					east=arg.num();
					if (east<-180.0 || east>180.0)
						printf("Option -x: <east> must be in range -180 to 180\n");
					arg=argv[i+4];
					north=arg.num();
					if (north<-90.0 || north>90.0)
						printf("Option -x: <north> must be in range -90 to 90\n");
					if (west>=-180.0 && west<=180.0 && east>=-180.0 && east<=180.0 && west>east)
						printf("Option -x: undefined window (<west> must be smaller than <east>)\n");
					if (south>=-90.0 && south<=90.0 && north>=-90.0 && north<=90.0 && south>north)
						printf("Option -x: undefined window (<south> must be smaller than <north>)\n");
					i+=4;
				}
				else {
					printf("Option -x must be followed by <lon> <lat> or <west> <south> <east> <north>\n");
					return false;
				}
				if (west<-180.0 || west<-180.0 || east<-180.0 || east>180.0 ||
					north<-90.0 || north>90.0 || south<-90.0 || south>90.0)
						return false;
				havewindow=true;
			}
			else if (arg=="-tab") {
				sep="\t";
			}
			else if (arg=="-eq") { // suppress area weighting
				ifweight=false;
			}
			else if (arg=="-n") { // suppress time step data
				ifyear=false;
			}
			else if (arg=="-h" || arg=="-help") printhelp(argv[0]);
			else if (arg="-sum") { 
				ifsum=true;
			}
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
	
	if (!havewindow) {
		west=-180.0;
		east=180.0;
		south=-90.0;
		north=90.0;
	}
	
	if (outfile=="") {
		
		xtring filepart=infile;
		stripfilename(filepart);
		
		if (havewindow) {
			if (north==south && east==west)
				outfile.printf("%s_%g_%g.txt",(char*)filepart,east,north);
			else
				outfile.printf("%s_%g_%g-%g_%g.txt",(char*)filepart,west,south,east,north);
		}
		else
			outfile.printf("%s_all.txt",(char*)filepart);
	}
	
	if ((yearitemno!=0 || yearitem!="") && !ifyear) {
		printf("Option -n cannot be combined with -y\n");
		return false;
	}
	
	return true;
}


int main(int argc,char* argv[]) {

	xtring infile,outfile,lonitem,latitem,yearitem,witem,header;
	int lonitemno,latitemno,yearitemno,witemno;
	float fromyear,toyear;
	double north,south,east,west;
	double pixx,pixy,pixdx,pixdy;
	bool havepixsize,havepixoffset;
	bool iffrom,ifto,iffast,ifweight,ifyear;
	Item items[MAXITEM];
	int nitem;
	xtring sep;
	bool ifsum;
	
	if (!processargs(argc,argv,infile,outfile,lonitem,latitem,yearitem,witem,
		lonitemno,latitemno,yearitemno,witemno,
		pixx,pixy,pixdx,pixdy,
		havepixsize,havepixoffset,
		north,south,east,west,
		fromyear,toyear,iffrom,ifto,sep,iffast,ifweight,ifyear,ifsum))
			abort(argv[0]);

	unixtime(header);
	header=(xtring)"[ASLICE  "+header+"]\n\n";
	printf("%s",(char*)header);

	if (readdata(infile,north,south,east,west,items,nitem,
		lonitemno,latitemno,yearitemno,witemno,lonitem,latitem,yearitem,witem,false,
		pixx,pixy,pixdx,pixdy,havepixsize,havepixoffset,ifweight,ifyear)) {
	
		if (writedata(outfile,items,nitem,lonitemno,latitemno,yearitemno,witemno,nyear,sep,ifyear,ifsum)) {
		
			printf("\n%d records written to %s\n\n",nyear,(char*)outfile);
		}
	} 
	
	return 0;
}
