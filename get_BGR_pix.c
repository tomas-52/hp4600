//trying to get detailed calibration values the geometry fixed scan is divided into 1-pixel vertical lines
//for each line the minimum and maximum RGB values are collected
//the calculations come from previous code for RGB values, so the B and R variables are interchanged !!!
//this is corrected only in the output !!!!!
//this gives in total 2x3x1274 values, the description below is there only to describe the scan properties
//file size is 1274 x 1760 pixels, 3822+2 bytes per line

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv){
	FILE * infile;
	unsigned char inbuf[3824];
	int x, i, j, m;
	int rmin[1274], gmin[1274], bmin[1274];
	int rmax[1274], gmax[1274], bmax[1274];

	for(x = 0; x < 1274; x++){
		rmin[x] = 255;
		gmin[x] = 255;
		bmin[x] = 255;
		rmax[x] = 0;
		gmax[x] = 0;
		bmax[x] = 0;
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
		for(i = 0; i < 3824; i+=3){
			x = i / 3;
			if(rmin[x] > inbuf[i]){ rmin[x] = inbuf[i];};
			if(gmin[x] > inbuf[i+1]){ gmin[x] = inbuf[i+1];};
			if(bmin[x] > inbuf[i+2]){ bmin[x] = inbuf[i+2];};
			if(rmax[x] < inbuf[i]){ rmax[x] = inbuf[i];};
			if(gmax[x] < inbuf[i+1]){ gmax[x] = inbuf[i+1];};
			if(bmax[x] < inbuf[i+2]){ bmax[x] = inbuf[i+2];};
		}
	}
	
	printf(" line   rmin   gmin   bmin   rmax   gmax   bmax\n");
	for(j = 0; j < 1274; j++){
		printf("%5d  %5d  %5d  %5d  %5d  %5d  %5d\n", j, bmin[j], gmin[j], rmin[j], bmax[j], gmax[j], rmax[j]);
	}
	pclose(infile);
}
