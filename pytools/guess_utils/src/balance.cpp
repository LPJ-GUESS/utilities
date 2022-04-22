
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
int start_year;
int end_year;
string pool_path;
string flux_path;
string type_of_matter;

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
	if (!options.count("-pool") ||
	    !options.count("-flux") ||
		!options.count("-start") ||
		!options.count("-end") ||
	    !options.count("-matter")) {
		return false;
	}

	// Assign to the global variables
	try {
		pool_path        =       options["-pool"];
		flux_path        =       options["-flux"];
		start_year       = toint(options["-start"]);
		end_year         = toint(options["-end"]);
		type_of_matter   =       options["-matter"];
	} catch (const ParseError& e) {
		printf("%s\n", e.what());
		return false;
	}

	// Make sure the given arguments are sane
	if (end_year <= start_year) {
		printf("Please make sure end year > start year\n");
		return false;
	}

	if (pool_path.length() == 0) {
		printf("Please supply a pool path\n");
		return false;
	}

	if (flux_path.length() == 0) {
		printf("Please supply a pool path\n");
		return false;
	}

	return true;
}

// Gives the user instructions on how to run the program
void printusage() {
	printf("Usage:\nbalance -pool <path> -flux <path> -start <year> -end <year> -matter <type>\n"\
	       "\n"\
	       "Options:\n"\
	       "\n"\
	       "-pool <path>\n"\
	       "\tPathname for file containing pool values\n"
	       "-flux <path>\n"\
	       "\tPathname for file containing flux values\n"\
	       "-start <year>\n"\
	       "\tDefines the starting year for which the balance is calculated\n"\
	       "-end <year>\n"\
	       "\tDefines the end year for which the balance is calculated\n"\
	       "-matter <type>\n"\
	       "\tThe type of matter (e.g. C or N), only used for descriptive purposes\n"\
	       "\n"\
	       "The input files are expected to be text files with exactly four columns,\n"\
	       "longitude, latitude, year and a value column. The value column should\n"\
	       "contain values in kg per square meter for the chosen type of matter,\n"\
	       "for instance:\n"\
	       "\n"\
	       " Lon  Lat Year  Total\n"\
	       "19.5 65.0  500 37.327\n"\
	       "19.5 65.0  501 37.377\n"\
	       "  .    .    .    .   \n"\
	       "  .    .    .    .   \n"\
	       "\n"\
	       "Output is written to two text files with results per grid cell in\n"\
	       "one file and totals in the other. The file names will depend on\n"\
	       "the chosen type of matter, for instance:\n"\
	       "\n"\
	       "Cbalance_gridcells.txt and Cbalance_totalerror_GtC.txt\n");
}


// Calculates area (in square km) of a grid cell
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

	double absdiff = 0.0;
	double uptake_pool = 0.0;
	double uptake_flux = 0.0;

	string balance_cell_file = type_of_matter + "balance_gridcells.txt";
	string balance_total_file = type_of_matter + "balance_totalerror_Gt" + type_of_matter + ".txt";


	FILE * in_pool = fopen(pool_path.c_str(),"rt"); // rt = read text
	FILE * in_flux = fopen(flux_path.c_str(),"rt"); // rt = read text
	
	FILE * out_balance_cell = fopen(balance_cell_file.c_str(),"wt"); // wt = write text
	FILE * out_balance_total = fopen(balance_total_file.c_str(),"wt"); // wt = write text
	

	if(!in_pool) {
		printf("Could not open input\n");
		return 1;
	}

	if(!in_flux) {
		printf("Could not open input\n");
		return 1;
	}

	if(!out_balance_cell) {
		printf("Could not open output\n");
		return 1;
	}

	if(!out_balance_total) {
		printf("Could not open output\n");
		return 1;
	}


	xtring pool_header,flux_header; // For reading in first column headings

	readfor(in_pool,"a#",&pool_header);
	readfor(in_flux,"a#",&flux_header);

	string inputstr = "f,f,i,f";

	// Headers in out files
	fprintf(out_balance_cell,"%10s%10s%20s%20s\n","lon","lat",(string("absdiff_kg")+type_of_matter+"m-2").c_str(),(string("error_kg")+type_of_matter).c_str());
	fprintf(out_balance_total,"%12s%12s%12s\n",(string("pool_Gt")+type_of_matter).c_str(),(string("flux_Gt")+type_of_matter).c_str(),(string("absdiff_Gt")+type_of_matter).c_str());

	double lon_current, lat_current;
	int years_read_for_current;
	bool has_current = false;

	double cell_pool_start, cell_pool_end;
	double cell_flux;

	while (!feof(in_pool) && !feof(in_flux)) {

		double lon_pool, lat_pool;
		double lon_flux, lat_flux;

		int year_pool, year_flux;

		bool end_of_input = false;

		double pool_value, flux_value;
		if (!readfor(in_pool,inputstr.c_str(),&lon_pool,&lat_pool,&year_pool,&pool_value) ||
		    !readfor(in_flux,inputstr.c_str(),&lon_flux,&lat_flux,&year_flux,&flux_value)) {
			end_of_input = true;
		}

		if (!end_of_input && 
		    (lon_pool != lon_flux ||
		     lat_pool != lat_flux ||
		     year_pool != year_flux)) {
			printf("Mismatched lines in pool and flux files!\n");
			return 1;
		}

		// time to finish a cell?
		if (has_current && (end_of_input || lon_pool != lon_current || lat_pool != lat_current)) {

			if (years_read_for_current != end_year-start_year+1) {
				printf("Didn't read all years for %.2f %.2f\n", lon_current, lat_current);
				return 1;
			}

			// Gridcell area only needs to be calculated once per cell
			double cellarea = pixelsize(lon_current, lat_current, 0.5, 0.5, 3); // km2

			// Total uptake from start_year to end_year
			double uptake_pool_cell = cell_pool_end-cell_pool_start;
			double absdiff_cell = fabs(-cell_flux-uptake_pool_cell); // error (kg/m2 in this gridcell)
		
			double absdiff_cell_kg = absdiff_cell * cellarea * 1000000.0; // kg/m2 to kg

			cell_flux *= cellarea * 1000000.0; // kg/m2 to kg
			uptake_pool_cell *= cellarea * 1000000.0; // kg/m2 to kg

			absdiff += absdiff_cell_kg / 1000000000000.0; // kg to Gt (=10**12 kg)
			uptake_pool += uptake_pool_cell / 1000000000000.0; // kg to Gt (=10**12 kg)
			uptake_flux += -cell_flux / 1000000000000.0; // kg to Gt (=10**12 kg)
		
			fprintf(out_balance_cell,"%10.2f%10.2f%20.4f%20.4f\n",lon_current,lat_current,absdiff_cell,absdiff_cell_kg);
		}

		if (end_of_input) {
			break;
		}

		// start of processing new grid cell?
		if (!has_current || lon_pool != lon_current || lat_pool != lat_current) {

			lon_current = lon_pool;
			lat_current = lat_pool;

			has_current = true;

			years_read_for_current = 0;
		}

		if (year_pool >= start_year && year_pool <= end_year) {
			if (year_pool-start_year != years_read_for_current) {
				printf("Unexpected year: %d (expected %d)\n", year_pool, start_year+years_read_for_current);
				return 1;
			}
			years_read_for_current++;
		}

		if (year_pool == start_year) {
			cell_pool_start = pool_value;
			cell_flux = 0.0;
		}

		if (year_pool == end_year) {
			cell_pool_end = pool_value;
		}

		if (year_pool > start_year && year_pool <= end_year) {
			cell_flux += flux_value;
		}
	}	

	// TOTAL error in the balance (Gt)
	fprintf(out_balance_total,"%12.6f%12.6f%12.6f\n",uptake_pool,uptake_flux,absdiff);


	// Close files
	fclose(in_pool);
	fclose(in_flux);

	fclose(out_balance_cell);
	fclose(out_balance_total);

	return 0;
}
