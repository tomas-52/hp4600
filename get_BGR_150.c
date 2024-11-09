//trying to get detailed calibration values the whole rotated scan is divided into 1-pixel vertical lines
//the calculations come from previous code for RGB values, so the B and R variables are interchanged !!!
//this is corrected only in the output !!!!!
//for each line the average RGB values are calculated, this gives 3x1274 values
//the description below is there only to describe the scan properties
//file size is 1274 x 1760 pixels, 3822+2 bytes per line

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv){
	FILE * infile;
	unsigned char inbuf[3824];
	float rval[1274];
	float gval[1274];
	float bval[1274];
	int x, i, j, m;
	unsigned int rsum[1274];
	unsigned int gsum[1274];
	unsigned int bsum[1274];

	for(x = 0; x < 1274; x++){
		rval[x] = 0.0;
		gval[x] = 0.0;
		bval[x] = 0.0;
		rsum[x] = 0;
		gsum[x] = 0;
		bsum[x] = 0;
	}

int linenum = 0;
	if (argc!=2) {
		printf("\n Usage: %s inputfile\n\n", argv[0]);
		exit(1);
	}

	if (!(infile = fopen(argv[1], "r")))
    	{
		printf("\nerror in the command line\nsource file does not exist\n\n");
		exit(1);
	}
	
	//reversed lines counting
	for(linenum = 5; linenum < 1755; linenum++){
		fseek(infile, (linenum*3824+54), SEEK_SET);
		fread(inbuf, 3824, 1, infile);
		for(i = 0; i < 3822; i+=3){
			x = i / 3;
			rsum[x] += (unsigned int)inbuf[i];
			gsum[x] += (unsigned int)inbuf[i+1];
			bsum[x] += (unsigned int)inbuf[i+2];
		}
	}
	for(j = 0; j < 1274; j++){
		rval[j] = (float) rsum[j] / 1750;
		gval[j] = (float) gsum[j] / 1750;
		bval[j] = (float) bsum[j] / 1750;
	}
	
	printf(" line     bval     gval     rval\n");
	for(j = 0; j < 1274; j++){
		printf("%5d    %5.1f    %5.1f    %5.1f\n", j, rval[j], gval[j], bval[j]);
	}
	
	pclose(infile);
}
