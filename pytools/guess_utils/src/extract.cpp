////////////////////////////////////////////////////////////////////////////////////////
// EXTRACT
// Postprocessing utility for LPJ-GUESS
// Takes raw ASCII output file with header row from LPJ-GUESS as input file
// Generates output file with records matching logical expression
//
// Written by Ben Smith
// This version dated 2006-05-10
//
// extract -help for documentation 

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <gutil.h>
#include <math.h>

const int MAXITEM=380;
	// Maximum number of items in a record (row) of an output file
	// (not including longitude,latitude,id and year)

///////////////////////////////////////////////////////////////////////////////////////////
// Global stuff used by the expression evaluator

const int MAXSTACK=1000; // Maximum number of tokens in expression and size of stack

typedef enum {NOTOKEN,NUMBER,UNARYMINUS,UNARYPLUS,OPMINUS,OPPLUS,OPMULTIPLY,OPDIVIDE,OPMOD,OPPOWER,
	OPGREATERTHAN,OPLESSTHAN,OPGREATEREQUAL,OPLESSEQUAL,OPEQUAL,OPNOTEQUAL,OPAND,OPOR,OPNOT,
	IDENTIFIER,FUNCTION,OPENPARENTHESIS,CLOSEPARENTHESIS} tokentype;
	
typedef enum {FUNCLOG10,FUNCLN,FUNCEXP,FUNCSIN,FUNCCOS,FUNCTAN,FUNCASIN,FUNCACOS,FUNCATAN,
	FUNCABS,FUNCINT,FUNCROUND,FUNCSQRT,FUNCPOW} functype;

struct Token {

	xtring token;
	tokentype type;
	double value; // used by extract
	int itemno; // used by extract
	functype func;
};

Token tlist[MAXSTACK]; // list of tokens
Token plist[MAXSTACK]; // list of tokens in reverse polish order

////////////////////////////////////////////////////////////////////////////////////////////
// Code for the expression evaluator

bool gettoken(xtring& text,int& pointer,xtring& token,tokentype& type,bool ifwaitop,int& nparen) {

	// ifwaitop = waiting for operator
	//            if true, we must be waiting for an operator or close parenthesis
	//            if false, we must be waiting for a number, identifier or open parenthesis
	// returns false on syntax error (ptr points to offending character)

	char ch;
	int len=text.len(),err=0;
	bool waitnum=false,done=false;
	bool waitdec,waitexp,waitexpsign,waitexpval=false,waitequal=false,waitident=false;
	bool waitand=false,waitor=false;
	int ptr=pointer;

	token="";

	while (ptr<len && !done) {

		ch=text[ptr];
		if (ch!=' ' && ch!='\t') {
			if (waitnum) {
				if (ch>='0' && ch<='9') {
					token+=ch;
					if (waitexpsign) waitexpsign=false;
					else if (waitexpval) waitexpval=false;
				}
				else if (ch=='.') {
					if (waitdec) {
						token+=ch;
						waitdec=false;
					}
					else {
						done=true;
						err=1;
					}
				}
				else if (ch=='e' || ch=='E') {
					if (waitexp) {
						token+=ch;
						waitexp=false;
						waitexpsign=true;
					}
					else {
						done=true;
						err=2;
					}
				}
				else if (ch=='-' || ch=='+') {
					if (waitexpsign) {
						token+=ch;
						waitexpsign=false;
						waitexpval=true;
					}
					else {
						if (waitexpval) err=3;
						done=true;
						type=NUMBER;
						ptr--;
					}
				}
				else {
					if (waitexpsign || waitexpval) {
						err=4;
					}
					else type=NUMBER;
					done=true;
					ptr--;
				}
			}
			else if (waitident) {
				if (ch=='(') {
					token+=ch;
					type=FUNCTION;
					done=true;
					nparen++;
				}
				else if (ch=='*' || ch=='/' || ch=='-' || ch=='+' || ch=='&' || ch=='|' || ch=='^' || ch=='=' || ch==')'
					|| ch=='>' || ch=='<' || ch=='!' || ch=='%') {
					ptr--;
					type=IDENTIFIER;
					done=true;
				}
				else {
					token+=ch;
				}
			}
			else if (waitequal) {
				if (ch=='=') {
					token+=ch;
					if (token=="<=") type=OPLESSEQUAL;
					else if (token==">=") type=OPGREATEREQUAL;
					else if (token=="==") type=OPEQUAL;
					else if (token=="!=") type=OPNOTEQUAL;
				}
				else {
					if (token=="<") type=OPLESSTHAN;
					else if (token==">") type=OPGREATERTHAN;
					else if (token=="=") type=OPEQUAL;
					ptr--;
				}
				done=true;
			}
			else if (waitand) {
				if (ch=='&') {
					token+=ch;
					type=OPAND;
				}
				else ptr--;
				done=true;
			}
			else if (waitor) {
				if (ch=='|') {
					token+=ch;
					type=OPOR;
				}
				else ptr--;
				done=true;
			}
			else if (ifwaitop) {
				if (ch=='-') {
					token=ch;
					type=OPMINUS;
					done=true;
				}
				else if (ch=='+') {
					token=ch;
					type=OPPLUS;
					done=true;
				}
				else if (ch=='*') {
					token=ch;
					type=OPMULTIPLY;
					done=true;
				}
				else if (ch=='/') {
					token=ch;
					type=OPDIVIDE;
					done=true;
				}
				else if (ch=='^') {
					token=ch;
					type=OPPOWER;
					done=true;
				}
				else if (ch=='%') {
					token=ch;
					type=OPMOD;
					done=true;
				}
				else if (ch=='>' || ch=='<' || ch=='=' || ch=='!') {
					token=ch;
					waitequal=true;
				}
				else if (ch=='&') {
					token=ch;
					waitand=true;
				}
				else if (ch=='|') {
					token=ch;
					waitor=true;
				}
				else if (ch==')') {
					token=ch;
					type=CLOSEPARENTHESIS;
					done=true;
					nparen--;
					if (nparen<0) {
						ptr--;
						err=5;
						done=true;
					}
				}
				else {
					ptr--;
					err=6;
					done=true;
				}
			}
			else {
				if (ch>='0' && ch<='9') {
					token+=ch;
					waitnum=true;
					waitdec=true;
					waitexp=true;
					waitexpsign=false;
				}
				else if (ch=='-') {
					token=ch;
					type=UNARYMINUS;
					done=true;
				}
				else if (ch=='+') {
					token=ch;
					type=UNARYPLUS;
					done=true;
				}
				else if (ch=='!') {
					token=ch;
					type=OPNOT;
					done=true;
				}
				else if (ch=='(') {
					token=ch;
					type=OPENPARENTHESIS;
					done=true;
					nparen++;
				}
				else if (ch==')') {
					ptr--;
					err=7;
					done=true;
				}
				else {
					token=ch;
					waitident=true;
				}
			}
		}

		ptr++;
	}

	if (err) {
		pointer=ptr;
		return false;
	}

	// else
	
	if (!done) {
		if (waitnum) {
			if (waitexpsign || token.right(1)=="-" || token.right(1)=="+") {
				pointer=ptr;
				return false;
			}
			else type=NUMBER;
		}
		else if (waitequal) {
			if (token=="<") type=OPLESSTHAN;
			else if (token==">") type=OPGREATERTHAN;
			else if (token=="=") type=OPEQUAL;
		}
		else if (waitident) type=IDENTIFIER;
		else if (waitand) type=OPAND;
		else if (waitor) type=OPOR;
		else {
			printf("Unexpected condition\n");
			exit(99);
		}
	}

	pointer=ptr;
	return true;
}


void buildplist(int level,int& tptr,int& sptr,int ntoken) {
	Token t,lastop;
	bool haveop=false,pushop=false;
	int oplevel;

	while (tptr<ntoken) {

		t=tlist[tptr++]; // get next token

		if (t.type==NUMBER || t.type==IDENTIFIER) { // push onto stack
			plist[sptr++]=t;
			pushop=true;
		}
		else if (t.type==OPENPARENTHESIS) { // push entire parenthesis onto stack
			buildplist(0,tptr,sptr,ntoken);
			pushop=true;
		}
		else if (t.type==FUNCTION) { // push function argument, then the function itself onto stack
			buildplist(0,tptr,sptr,ntoken);
			plist[sptr++]=t;
			pushop=true;
		}
		else if (t.type==OPAND || t.type==OPOR || t.type==OPGREATERTHAN || t.type==OPLESSTHAN ||
			t.type==OPGREATEREQUAL || t.type==OPLESSEQUAL || t.type==OPEQUAL || t.type==OPNOTEQUAL || t.type==OPPLUS ||
			t.type==OPMINUS || t.type==OPMULTIPLY || t.type==OPDIVIDE || t.type==OPPOWER || t.type==OPMOD) { // binary operators

			if (t.type==OPOR) oplevel=1;
			if (t.type==OPAND) oplevel=2;
			else if (t.type==OPGREATERTHAN || t.type==OPLESSTHAN || t.type==OPGREATEREQUAL || t.type==OPLESSEQUAL
				|| t.type==OPEQUAL || t.type==OPNOTEQUAL) oplevel=3;
			else if (t.type==OPPLUS || t.type==OPMINUS || t.type==OPMOD) oplevel=4;
			else if (t.type==OPMULTIPLY || t.type==OPDIVIDE) oplevel=5;
			else if (t.type==OPPOWER) oplevel=6;

			if (oplevel<=level) {
				tptr--;
				return;
			}
			else {
				buildplist(oplevel,tptr,sptr,ntoken);
				pushop=true;
			}

			lastop=t;
			haveop=true;
		}
		else if (t.type==UNARYMINUS || t.type==UNARYPLUS || t.type==OPNOT) { // unary operators
			
			oplevel=7;
			
			buildplist(oplevel,tptr,sptr,ntoken);
			plist[sptr++]=t;
			pushop=true;
		}
		else if (t.type==CLOSEPARENTHESIS) {

			if (level>0) tptr--;
			return;
		}

		if (pushop) {

			if (haveop) {
				plist[sptr++]=lastop;
				haveop=false;
			}
			pushop=false;
		}
	}
}

char* strtoken(tokentype t) {

	switch (t) {

		case NOTOKEN: return "NOTOKEN";
				 break;
		case NUMBER: return "NUMBER";
				break;
		case UNARYMINUS: return "UNARYMINUS";
					break;
		case UNARYPLUS: return "UNARYPLUS";
		case OPMINUS: return "OPMINUS";
		case OPPLUS: return "OPPLUS";
		case OPMULTIPLY: return "OPMULTIPLY";
		case OPDIVIDE: return "OPDIVIDE";
		case OPMOD: return "OPMOD";
		case OPPOWER: return "OPPOWER";
		case OPGREATERTHAN: return "OPGREATERTHAN";
		case OPLESSTHAN: return "OPLESSTHAN";
		case OPGREATEREQUAL: return "OPGREATEREQUAL";
		case OPLESSEQUAL: return "OPLESSEQUAL";
		case OPEQUAL: return "OPEQUAL";
		case OPNOTEQUAL: return "OPNOTEQUAL";
		case OPAND: return "OPAND";
		case OPOR: return "OPOR";
		case OPNOT: return "OPNOT";
		case IDENTIFIER: return "IDENTIFIER";
		case FUNCTION: return "FUNCTION";
		case OPENPARENTHESIS: return "OPENPARENTHESIS";
		case CLOSEPARENTHESIS: return "CLOSEPARENTHESIS";

	}

	return "";
}
bool parse_expression(xtring expression,int& ntoken) {

	// Parses expression, transferring 'ntoken' tokens to reverse Polish list in
	// plist
	
	// Returns false on error
	
	int i,ptr=0,tptr=0,sptr=0;
	int len=expression.len();
	int nparen=0;
	bool ifwaitop=false;
	xtring token;
	tokentype type;
	
	while (ptr<len) {

		if (!gettoken(expression,ptr,token,type,ifwaitop,nparen)) {
			printf("Error in expression\n %s\n ",(char*)expression);
			for (i=0;i<ptr;i++) printf(" ");
			printf("^ at this point\n");
			return false;
		}

		tlist[tptr].token=token;
		tlist[tptr].type=type;
		tptr++;

		if (type==NUMBER || type==IDENTIFIER || type==CLOSEPARENTHESIS) ifwaitop=true;
		else ifwaitop=false;
	}

	if (nparen!=0) {
		printf("Error in expression\n %s\n ",(char*)expression);
		for (i=0;i<len;i++) printf(" ");
		printf("^ at this point\n");
		printf("Unbalanced parenthesis\n");
		return false;
	}
	
	ntoken=tptr;
	tptr=0;
	buildplist(0,tptr,sptr,ntoken);
	
	ntoken=sptr;
	
	return true;
}


bool evaluate(int ntoken,double* dval,double& val,int recno) {

	// Evaluates expression in plist based on data in dval
	
	double stack[MAXSTACK],result;
	int i,ptr=0;
	tokentype t;
	functype f;
	
	for (i=0;i<ntoken;i++) {
	
		t=plist[i].type;
		
		switch (t) {
		case NUMBER:
			stack[ptr++]=plist[i].value;
			break;
		case IDENTIFIER:
			if (plist[i].itemno>=0) stack[ptr++]=dval[plist[i].itemno];
			else stack[ptr++]=recno;
			break;
		case UNARYMINUS:
			stack[ptr-1]=-stack[ptr-1];
			break;
		case UNARYPLUS:
			// nothing to do
			break;
		case OPNOT:
			stack[ptr-1]=!stack[ptr-1];
			break;
		case OPMINUS:
			result=stack[ptr-2]-stack[ptr-1];
			stack[ptr-2]=result;
			ptr--;
			break;
		case OPPLUS:
			result=stack[ptr-2]+stack[ptr-1];
			stack[ptr-2]=result;
			ptr--;
			break;
		case OPMULTIPLY:
			result=stack[ptr-2]*stack[ptr-1];
			stack[ptr-2]=result;
			ptr--;
			break;
		case OPDIVIDE:
			result=stack[ptr-2]/stack[ptr-1];
			stack[ptr-2]=result;
			ptr--;
			break;
		case OPMOD:
			result=fmod(stack[ptr-2],stack[ptr-1]);
			stack[ptr-2]=result;
			ptr--;
			break;
		case OPPOWER:
			if (stack[ptr-1]==3.0) result=stack[ptr-2]*stack[ptr-2]*stack[ptr-2];
			else if (stack[ptr-1]=2.0) result=stack[ptr-2]*stack[ptr-2];
			else result=pow(stack[ptr-2],stack[ptr-1]);
			stack[ptr-2]=result;
			ptr--;
			break;
		case OPGREATERTHAN:
			result=stack[ptr-2]>stack[ptr-1];
			stack[ptr-2]=result;
			ptr--;
			break;
		case OPLESSTHAN:
			result=stack[ptr-2]<stack[ptr-1];
			stack[ptr-2]=result;
			ptr--;
			break;
		case OPGREATEREQUAL:
			result=stack[ptr-2]>=stack[ptr-1];
			stack[ptr-2]=result;
			ptr--;
			break;
		case OPLESSEQUAL:
			result=stack[ptr-2]<=stack[ptr-1];
			stack[ptr-2]=result;
			ptr--;
			break;
		case OPEQUAL:
			result=stack[ptr-2]==stack[ptr-1];
			stack[ptr-2]=result;
			ptr--;
			break;
		case OPNOTEQUAL:
			result=stack[ptr-2]!=stack[ptr-1];
			stack[ptr-2]=result;
			ptr--;
			break;
		case OPAND:
			result=stack[ptr-2] && stack[ptr-1];
			stack[ptr-2]=result;
			ptr--;
			break;
		case OPOR:
			result=stack[ptr-2] || stack[ptr-1];
			stack[ptr-2]=result;
			ptr--;
			break;
		case FUNCTION:
			f=plist[i].func;
			switch (f) {
			case FUNCLOG10:
				result=log10(stack[ptr-1]);
				break;
			case FUNCLN:
				result=log(stack[ptr-1]);
				break;
			case FUNCEXP:
				result=exp(stack[ptr-1]);
				break;
			case FUNCSIN:
				result=sin(stack[ptr-1]);
				break;
			case FUNCCOS:
				result=cos(stack[ptr-1]);
				break;
			case FUNCTAN:
				result=tan(stack[ptr-1]);
				break;
			case FUNCASIN:
				result=asin(stack[ptr-1]);
				break;
			case FUNCACOS:
				result=acos(stack[ptr-1]);
				break;
			case FUNCATAN:
				result=atan(stack[ptr-1]);
				break;
			case FUNCABS:
				result=fabs(stack[ptr-1]);
				break;
			case FUNCINT:
				result=floor(stack[ptr-1]);
				break;
			case FUNCROUND:
				result=floor(stack[ptr-1]+0.5);
				break;
			case FUNCSQRT:
				result=sqrt(stack[ptr-1]);
				break;
			default:
				printf("Unexpected error\n");
				return false;
			}
			stack[ptr-1]=result;
			break;
		default:
			printf("Unexpected error*\n");
			return false;
		}
	}
	
	if (ptr!=1) {
		printf("Unexpected error**\n");
		return false;
	}
	
	val=stack[0];
	
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////
// Code for EXTRACT

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


bool readheader(FILE*& in,xtring& whole_line,Item* items,int& ncol,
	xtring filename,
	double values[MAXITEM],bool& ifvalues,int& lineno) {

	// Reads header row of an LPJ-GUESS output file
	// label = array of header labels
	// ncol  = number of columns (labels)
	// Returns false if too many items in file
	
	xtring item,line;
	int pos,i;
	bool alphabetics=false;
	ncol=0;
	
	while (!ncol && !feof(in)) {
	
		readfor(in,"a#",&whole_line);
		line=whole_line;
		lineno++;
	
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

bool readrecord(FILE*& in,xtring& whole_line,double* dval,int nitem,Item* items,
	int& lineno,xtring& filename,bool iffast,bool warn) {

	// Reads one record (row) in output file
	// Returns false on end of file
	// nitem = total number of items
	
	xtring sval[MAXITEM],line;
	int i,places,digits,pos;
	bool ifsign,searching=true,blank,isnum;

	while (searching) {
	
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
		
		if (blank) {
			if (warn)
				printf("Line %d of %s is blank - ignoring\n",lineno,(char*)filename,feof(in));
		}
		else if (!isnum) {
			if (warn)
				printf("Line %d of %s contains non-numeric data - ignoring entire line\n",
					lineno,(char*)filename);
		}
		else {
			for (i=0;i<nitem;i++) {
				dval[i]=sval[i].num();
				if (!iffast) {
					if (scanitem(sval[i],places,digits,ifsign)) {
						if (places>items[i].places) items[i].places=places;
						if (digits>items[i].digits) items[i].digits=digits;
						if (ifsign) items[i].ifsign=true;
					}
					else items[i].ifnum=false;
				}
				searching=false;
			}
		}
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

bool convert_plist(int ntoken,xtring infile,Item* items,int nitem) {

	// Replaces identifiers in reverse Polish token list with item numbers
	// and string numerals with numbers
	
	int i;
	int itemno;
	xtring text;
	double dval;
	functype f;	
	
	for (i=0;i<ntoken;i++) {
		if (plist[i].type==IDENTIFIER) {
			if (plist[i].token[0]=='#') { // column number
				text=plist[i].token.mid(1);
				dval=text.num();
				if (!text.isnum() || dval!=int(dval) || dval<0) {
					printf("Error in expression: %s not a valid column number\n",
						(char*)plist[i].token);
					return false;
				}
				if (dval>nitem) {
					printf("Error in expression: %s\n",(char*)plist[i].token);
					printf("Only %d items in %s\n",nitem,(char*)infile);
					return false;
				}
				plist[i].itemno=dval-1;
			}
			else {
				itemno=0;
				if (!finditem(plist[i].token,itemno,infile,items,nitem)) return false;
				plist[i].itemno=itemno;
			}
		}
		else if (plist[i].type==NUMBER) {
			if (!plist[i].token.isnum()) {
				printf("Error in expression: cannot interpret %s as a number\n",
					(char*)plist[i].token);
				return false;
			}
			plist[i].value=plist[i].token.num();
		}
		else if (plist[i].type==FUNCTION) {
			text=plist[i].token.lower();
			if (text=="log10(") f=FUNCLOG10;
			else if (text=="ln(" || text=="log(") f=FUNCLN; 
			else if (text=="exp(") f=FUNCEXP; 
			else if (text=="sin(") f=FUNCSIN; 
			else if (text=="cos(") f=FUNCCOS; 
			else if (text=="tan(") f=FUNCTAN; 
			else if (text=="asin(") f=FUNCASIN; 
			else if (text=="acos(") f=FUNCACOS; 
			else if (text=="atan(") f=FUNCATAN; 
			else if (text=="abs(" || text=="fabs(") f=FUNCABS; 
			else if (text=="int(" || text=="floor(") f=FUNCINT; 
			else if (text=="round(") f=FUNCROUND; 
			else if (text=="sqrt(") f=FUNCSQRT; 
			else if (text=="pow(") f=FUNCPOW; 
			else {
				printf("Error in expression: unknown function %s)\n",(char*)plist[i].token);
				return false;
			}
			plist[i].func=f;
		}
	}
	
	return true;
}


bool readdata(xtring infile,xtring outfile,Item* items,int ntoken,int& nitem,int& nrec,
	bool iffast,xtring sep) {

	int inrec=0,i;
	double dval[MAXITEM],dval0[MAXITEM],thisval;
	bool ifvalues;
	int lineno=0,firstdata;
	xtring line;
	nrec=0;
	
	FILE* in=fopen(infile,"rt");
	if (!in) {
		printf("Could not open %s for input\n",(char*)infile);
		return false;
	}
	
	FILE* out=fopen(outfile,"wt");
	if (!out) {
		printf("Could not open %s for output\n",(char*)outfile);
		return false;
	}

	// Read header and find columns containing lon, lat, year
	if (!readheader(in,line,items,nitem,infile,dval0,ifvalues,lineno)) {
		return false;
	}
	
	firstdata=lineno;

	if (!convert_plist(ntoken,infile,items,nitem)) return false;
	
	if (iffast) fprintf(out,"%s\n",(char*)line);

	printf("Reading data from %s ...\n",(char*)infile);
	
	// Transfer data from first row (if all numbers)
	
	if (ifvalues && iffast) {
		
		if (!evaluate(ntoken,dval0,thisval,inrec+1)) return false;
		if (thisval) {
			fprintf(out,"%s\n",(char*)line);
			nrec++;
		}
		inrec++;
	}
		
	while (!feof(in)) {
		
		// Read next record in file
		
		if (readrecord(in,line,dval,nitem,items,lineno,infile,iffast,true)) {

			if (iffast) {
				if (!evaluate(ntoken,dval,thisval,inrec+1)) return false;
				
				if (thisval) {
					fprintf(out,"%s\n",(char*)line);
					nrec++;
				}
			}
			inrec++;
			
			if (!(inrec%50000)) printf("%d ...\n",inrec);
		}
	}
	
	// Slow mode
	
	if (!iffast) {
	
		// Print header row
		
		for (i=0;i<nitem;i++) {
			items[i].compute_fmt();
			if (i) fprintf(out,sep);
			fprintf(out,items[i].lfmt,(char*)items[i].label);
		}
		fprintf(out,"\n");
		
		// Reread input file, printing data as we go
		
		rewind(in);
		for (i=0;i<firstdata;i++) readfor(in,"");
		
		inrec=nrec=0;
		
		if (ifvalues) {
		
			if (!evaluate(ntoken,dval0,thisval,inrec+1)) return false;
			if (thisval) {
				for (i=0;i<nitem;i++) {
					if (i) fprintf(out,sep);
					fprintf(out,items[i].fmt,dval0[i]);
				}
				fprintf(out,"\n");
				nrec++;
			}
			inrec++;
		}
		
		while (!feof(in)) {
		
			// Read next record in file
			
			if (readrecord(in,line,dval,nitem,items,lineno,infile,iffast,false)) {
	
				if (!evaluate(ntoken,dval,thisval,inrec+1)) return false;
					
				if (thisval) {
					for (i=0;i<nitem;i++) {
						if (i) fprintf(out,sep);
						fprintf(out,items[i].fmt,dval[i]);
					}
					fprintf(out,"\n");
					nrec++;
				}
				inrec++;
			
				if (!(inrec%50000)) printf("%d ...\n",inrec);
			}
		}
	}
	
	fclose(in);
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

	fprintf(out,"EXTRACT\n");
	fprintf(out,"Extracts records matching a logical expression from a plain-text input file\n\n");
	fprintf(out,"Usage: %s <input-file> <options>\n\n",(char*)exe);
	fprintf(out,"Options:\n\n");
	fprintf(out,"-x <logical-expression>\n\n");
	fprintf(out,"    Logical expression describing records to extract to output file\n\n");
	fprintf(out,"    C/C++ syntax should be employed, referencing item values in <input-file>\n");
	fprintf(out,"    by item name as given in the header row (if present) or 1-based column number\n");
	fprintf(out,"    prefixed by '#' (e.g. '#1' refers to value in first column)\n\n");
	fprintf(out,"    Record numbers (1-based) may be referenced by '#0'\n\n");
	fprintf(out,"    Item names must not include characters interpretable as operators, e.g. '-'\n\n");
	fprintf(out,"    The entire expression should normally be enclosed in single quotes,\n");
	fprintf(out,"    e.g. 'lat>=55.5 && lat<=72'\n\n");
	fprintf(out,"-o <output-file>\n");
	fprintf(out,"    Pathname for output file\n\n");
	fprintf(out,"-tab\n");
	fprintf(out,"    Tab-delimited output\n\n");
	fprintf(out,"-fast\n");
	fprintf(out,"    Fast mode\n\n");
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
	printf("Options: -x <logical-expression>\n");
	printf("         -o <output-file>\n");
	printf("         -tab\n");
	printf("         -fast\n");
	printf("         -help\n");

	exit(99);
}

bool processargs(int argc,char* argv[],xtring& infile,xtring& outfile,xtring& expression,
	bool& iffast,xtring& sep) {

	int i;
	xtring arg;
	bool haveinfile=false;
	
	// Defaults
	outfile="";
	expression="1"; // include all data by default
	sep=" ";
	iffast=false;
	
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
			else if (arg=="-x") { // logical expression
				if (argc>=i+2) {
					expression=argv[i+1];
				}
				else {
					printf("Option -x must be followed by a logical expression\n");
					return false;
				}
				i+=1;
			}
			else if (arg=="-h" || arg=="-help") printhelp(argv[0]);
			else if (arg=="-tab") {
				sep="\t";
			}
			else if (arg=="-fast") {
				iffast=true;
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
	
	if (outfile=="") {
		
		xtring filepart=infile;
		stripfilename(filepart);

		outfile.printf("%s_extract.txt",(char*)filepart);
	}
	
	if (sep=="\t") iffast=false; // Can't combine fast mode and tabs
	
	return true;
}


int main(int argc,char* argv[]) {

	xtring infile,outfile,expression,header,sep;
	Item items[MAXITEM];
	int nitem,nrec,ntoken;
	bool iffast;
		
	if (!processargs(argc,argv,infile,outfile,expression,iffast,sep)) abort(argv[0]);
	
	if (parse_expression(expression,ntoken)) { 
	
		unixtime(header);
		header=(xtring)"[EXTRACT  "+header+"]\n\n";
		printf("%s",(char*)header);
	
		if (readdata(infile,outfile,items,ntoken,nitem,nrec,iffast,sep)) {
			
			printf("\n%d records written to %s\n\n",nrec,(char*)outfile);
		} 
	}
		
	return 0;
}
