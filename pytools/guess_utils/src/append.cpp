////////////////////////////////////////////////////////////////////////////////////////
// APPEND
// Postprocessing utility for LPJ-GUESS
// Concatenates (joins end to end) two or more ASCII text files, retaining an initial
// header row (if present in the first file) and omitting header rows in subsequent
// files and blank rows in any of the files
//
// Written by Ben Smith
// This version dated 2006-06-22
//
// append -help for documentation 

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <gutil.h>

const int MAXFILE=100;
	// Maximum number of files to append


void readline(FILE*& in,xtring& text) {

	char ch;
	text="";
	while (!feof(in)){
		ch=getc(in);
		if (ch=='\n' || feof(in)) return;
		if (ch!='\r') text+=ch;
	} 
}
	
bool readwritedata(xtring* infile,int ninfile,xtring outfile,bool ifstrip,bool ifchain) {
	
	xtring text;
	bool firstline;
	FILE* in,*out;
	int j,pos,first;
	
	if (ifchain) {
		out=fopen(infile[0],"at");
		first=1;
	}
	else {
		out=fopen(outfile,"wt");
		first=0;
	}
	if (!out) {
		printf("Could not open %s for output\n",(char*)outfile);
		return false;
	}

	for (j=first;j<ninfile;j++) {
		in=fopen(infile[j],"rt");
		if (!in) {
			printf("Could not open %s for input\n",(char*)infile[j]);
			fclose(out);
			return false;
		}
		printf("Reading data from %s\n",(char*)infile[j]);
		firstline=true;
		while (!feof(in)) {
			readline(in,text);
			if (ifstrip) {
				if (firstline && text.findnotoneof(" \t")!=-1) {
					pos=text.findnotoneof(" .-\t");
					if (pos>=0)
						if (text[pos]>='0' && text[pos]<='9') firstline=false;
				}
				if (firstline && j) readline(in,text);
			}
			firstline=false;
			if (text.findnotoneof(" \t")!=-1 || !ifstrip) {
				if (text!="" || !feof(in))
					fprintf(out,"%s\n",(char*)text);
			}
		}
		fclose(in);
	}
	
	fclose(out);
	
	printf("\nOutput is in %s\n\n",(char*)outfile);
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

	fprintf(out,"APPEND\n");
	fprintf(out,"Concatenates two or more plain text input files.\n\n");
	fprintf(out,"Usage: %s <input-file> { <input-file> } <options>\n\n",(char*)exe);
	fprintf(out,"Options:\n");
	fprintf(out,"-o <output-file>\n");
	fprintf(out,"    Pathname for output file\n");
	fprintf(out,"-c\n");
	fprintf(out,"    Chain second and subsequent input files at end of first input file\n");
	fprintf(out,"    in list (cannot be combined with -o)\n");
	fprintf(out,"-n\n");
	fprintf(out,"    Suppresses purging of header row (if present) in second and subsequent\n");
	fprintf(out,"    input file and purging of blank lines in all input files\n");
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

	printf("Usage: %s <input-file> { <input-file> } <options>\n",(char*)exe);
	printf("Options: -o <output-file>\n");
	printf("         -c\n");
	printf("         -n\n");
	printf("         -help\n");

	exit(99);
}

bool processargs(int argc,char* argv[],xtring* infile,xtring& outfile,int& ninfile,
	bool& ifstrip,bool& ifchain) {

	int i;
	xtring arg;
	bool slut;
	double dval;
	
	// Defaults
	outfile="";
	ninfile=0;
	ifstrip=true;
	ifchain=false;
	
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
			else if (arg=="-h" || arg=="-help") printhelp(argv[0]);
			else if (arg=="-n") ifstrip=false;
			else if (arg=="-c") ifchain=true;
			else {
				printf("Invalid option %s\n",(char*)arg);
				return false;
			}
		}
		else {
			if (ninfile==MAXFILE) {
				printf("Too many input files specified\n");
				return false;
			}
			infile[ninfile++]=arg;
		}
	}
	
	if (ninfile<1) {
		printf("File or pathname for at least one input file must be specified\n");
		return false;
	}
	
	if (ifchain && outfile!="") {
		printf("Cannot combine -c with -o\n");
		return false;
	}
	
	if (outfile=="") {
		
		if (ifchain) outfile=infile[0];
		else {
			xtring filepart=infile[0];
			stripfilename(filepart);
		
			outfile.printf("%s_app.txt",(char*)filepart);
		}
	}
	
	return true;
}


int main(int argc,char* argv[]) {

	xtring infile[MAXFILE],outfile,header;
	int ninfile;
	bool ifstrip,ifchain;
	
	if (!processargs(argc,argv,infile,outfile,ninfile,ifstrip,ifchain)) abort(argv[0]);

	unixtime(header);
	header=(xtring)"[APPEND  "+header+"]\n\n";
	printf("%s",(char*)header);
	
	readwritedata(infile,ninfile,outfile,ifstrip,ifchain);
	
	return 0;
}
