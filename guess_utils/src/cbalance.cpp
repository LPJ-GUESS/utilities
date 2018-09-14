
#include <stdio.h> // standard input/output libary in C++
#include <time.h> // used in gutil.h
#include <string.h> // used in gutil.h
#include <gutil.h> // Ben's input/output utility
#include <math.h> 
#include <map>
#include <string>
#include <sstream>
#include <stdexcept>

using std::map;
using std::string;

// The options given to the program will be placed in these:
int num_spinup_years;
int num_cells;
int start_year;
int end_year;
string basedir;

/*
* This function parses a file header. Given the header "xxx yyy zzz" and
* the string "yyy", it will find out that there are 3 columns and that
* the yyy column has index 1.
*/
void parseheaders(const string& header, const string& interestingstr, 
				  int& ncolumns, int& interestingcolumn) {
	std::istringstream is(header);
	ncolumns = 0;
	interestingcolumn = -1;

	string name;
	while (is >> name) {
		ncolumns++;
		if (name == interestingstr) {
			interestingcolumn = ncolumns-1;
		}
	}
}

// An error thrown by the toint function when given bad input
class ParseError : public std::runtime_error {
public:
	ParseError(const string& msg) : std::runtime_error(msg) {}
};

// The toint function converts a string to an integer
int toint(const string& str) {
	char* pend;
	const char* c_str = str.c_str();

	int result = strtol(c_str, &pend, 10);
	if (pend == c_str+str.length()) {
		return result;
	}
	throw ParseError("Failed to parse: " + str);
}

/*
* This function handles the command line arguments.
* It will make sure all options are supplied by the user and place
* their values in the corresponding global variables. If there is
* some error in the arguments given this function will return false.
*/
bool handleargs(int argc, char** argv) {
	map<string, string> options;

	if (argc != 11) {
		return false;
	}

	// Go through the args two at a time, handling each option/value pair
	for (int i = 1; i < argc; i+=2) {
		string option(argv[i]);
		string value(argv[i+1]);

		if (option[0] != '-') {
			// Not a proper option
			return false;
		}
		options[option] = value;
	}

	// Make sure we got them all
	if (!options.count("-spinup") ||
		!options.count("-ncells") ||
		!options.count("-path") ||
		!options.count("-start") ||
		!options.count("-end")) {
		return false;
	}

	// Assign to the global variables
	try {
		num_spinup_years = toint(options["-spinup"]);
		num_cells        = toint(options["-ncells"]);
		basedir          =       options["-path"];
		start_year       = toint(options["-start"]);
		end_year         = toint(options["-end"]);
	} catch (const ParseError& e) {
		printf("%s\n", e.what());
		return false;
	}

	// Make sure the given arguments are sane
	if (start_year < num_spinup_years || end_year <= start_year) {
		printf("Please make sure start year >= spinup and end year > start year\n");
		return false;
	}

	if (basedir.length() == 0) {
		printf("Please supply a path\n");
		return false;
	}

	char last_char = basedir[basedir.length()];
	if (last_char != '\\' && last_char != '/') {
		basedir += "/";
	}

	return true;
}

// Gives the user instructions on how to run the program
void printusage() {
	printf("Usage:\ncbalance -spinup <years> -ncells <number_of_cells>\n");
	printf("\t-path <directory> -start <year> -end <year>\n");
}

// calculates grazed area per continent

double pixelsize(double longpos,double latpos,double longsize,double latsize,int postype) {

//c     (Ben Smith, 15/5/97)
//
//c     Returns area in square km of a pixel of a given size at a given point
//c     on the world.  The formula applied is the surface area of a segment of
//c     a hemisphere of radius r from the equator to a parallel (circular)
//c     plane h vertical units towards the pole: S=2*pi*r*h
//
//c     longpos   longitude position (see postype)
//c     latpos    latitude position (see postype)
//c     longsize  longitude range in degrees
//c     latsize   latitude range in degrees
//c     postype   declares which part of the pixel longpos and latpos
//c               refer to:
//c               0 = centre
//c               1 = NW corner
//c               2 = NE corner
//c               3 = SW corner
//c               4 = SE corner

	double pi,r,h1,h2,lattop,latbot,s;
      
	pi=3.1415926536;
	r=6367.425;   //mean radius of the earth

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



int main(int argc, char** argv) {

	if (!handleargs(argc, argv)) {
		printusage();
		return 1;
	}

	double absCdiff = 0.0;
	double uptake_cpool = 0.0;
	double uptake_cflux = 0.0;

	double lon,lat,year; 

	string cpool_file = basedir + "cpool.out";
	string cflux_file = basedir + "cflux.out";
	
	string cbalance_cell_file = basedir + "cbalance_gridcells.txt";
	string cbalance_total_file = basedir + "cbalance_totalerror_GtC.txt";


	FILE * in_cpool = fopen(cpool_file.c_str(),"rt"); // rt = read text
	FILE * in_cflux = fopen(cflux_file.c_str(),"rt"); // rt = read text
	
	FILE * out_cbalance_cell = fopen(cbalance_cell_file.c_str(),"wt"); // wt = write text
	FILE * out_cbalance_total = fopen(cbalance_total_file.c_str(),"wt"); // wt = write text
	

	if(!in_cpool) {
		printf("Could not open input\n");
		return 1;
	}

	if(!in_cflux) {
		printf("Could not open input\n");
		return 1;
	}

	if(!out_cbalance_cell) {
		printf("Could not open output\n");
		return 1;
	}

	if(!out_cbalance_total) {
		printf("Could not open output\n");
		return 1;
	}



	xtring cpool_header,cflux_header; // For reading in first column headings

	readfor(in_cpool,"a#",&cpool_header);
	readfor(in_cflux,"a#",&cflux_header);

	int cpool_columns, cflux_columns;
	int total_column, nee_column;

	parseheaders((char*)cpool_header, "Total", cpool_columns, total_column);
	parseheaders((char*)cflux_header, "NEE", cflux_columns, nee_column);

	if (total_column == -1) {
		printf("No Total column in cpool file\n");
		return 1;
	}
	if (nee_column == -1) {
		printf("No NEE column in cflux file\n");
		return 1;
	}

	// Allocate arrays for the data from each row other than lon, lat and year
	double* cpool_data = new double[cpool_columns-3];
	double* cflux_data = new double[cflux_columns-3];

	// Adjust total_column and nee_column for the arrays
	total_column -= 3;
	nee_column   -= 3;

	xtring cpool_inputstr, cflux_inputstr;
	cpool_inputstr.printf("f,f,f,%df", cpool_columns-3);
	cflux_inputstr.printf("f,f,f,%df", cflux_columns-3);

	// Header in cell C balance file
	fprintf(out_cbalance_cell,"%10s%10s%20s%20s\n","lon","lat","absCdiff_kgCm-2","error_kgC");
	fprintf(out_cbalance_total,"%12s%12s%12s\n","cpool_GtC","cflux_GtC","absdiff_GtC");


	// GRIDCELL LOOP

	for (long cell = 0; cell < num_cells; cell++) {


		double cell_cpool_start = 0.0;
		double cell_cpool_end   = 0.0;
		
		double cell_cflux = 0.0;

		double cellarea = 0.0;


		// YEAR LOOP, for this gridcell. 

		for (int yr = start_year; yr <= end_year; yr++) {
			
			// CPOOL
			readfor(in_cpool,(char*)cpool_inputstr,&lon,&lat,&year,cpool_data);
			
			// start year
			if (int(year) == start_year) {
				cell_cpool_start = cpool_data[total_column];
			
				// Gridcell area only needs to be calculated once per cell
				cellarea = pixelsize(lon,lat,0.5,0.5,3); // km2	
			}

			// end year
			if (int(year) == end_year)
				cell_cpool_end = cpool_data[total_column];

			
			// CFLUX
			readfor(in_cflux,(char*)cflux_inputstr,&lon,&lat,&year,cflux_data);

			if (int(year) == start_year)
				cell_cflux = 0.0;
			else
				cell_cflux += cflux_data[nee_column]; // Sum total NEE from start_year+1 to end_year, inclusive
		}

		// Total uptake from start_year to end_year
		double uptake_cpool_cell = cell_cpool_end-cell_cpool_start;
		double absCdiff_cell = fabs(-cell_cflux-uptake_cpool_cell); // error (kgC/m2 in this gridcell)
		
		double absCdiff_cell_kgC = absCdiff_cell * cellarea * 1000000.0; // kgC/m2 to kgC

		cell_cflux *= cellarea * 1000000.0; // kgC/m2 to kgC
		uptake_cpool_cell *= cellarea * 1000000.0; // kgC/m2 to kgC

		absCdiff += absCdiff_cell_kgC / 1000000000000.0; // kgC to GtC (=10**12 kgC)
		uptake_cpool += uptake_cpool_cell / 1000000000000.0; // kgC to GtC (=10**12 kgC)
		uptake_cflux += -cell_cflux / 1000000000000.0; // kgC to GtC (=10**12 kgC)
		
		fprintf(out_cbalance_cell,"%10.2f%10.2f%20.4f%20.4f\n",lon,lat,absCdiff_cell,absCdiff_cell_kgC);

	}

	delete[] cpool_data;
	delete[] cflux_data;

	// TOTAL error in the C balance (GtC)
	fprintf(out_cbalance_total,"%12.6f%12.6f%12.6f\n",uptake_cpool,uptake_cflux,absCdiff);


	// Close files
	fclose(in_cpool);
	fclose(in_cflux);

	fclose(out_cbalance_cell);
	fclose(out_cbalance_total);

	return 0;
}
