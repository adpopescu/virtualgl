/* Copyright (C)2014, 2017 D. R. Commander
 *
 * This library is free software and may be redistributed and/or modified under
 * the terms of the wxWindows Library License, Version 3.1 or (at your option)
 * any later version.  The full license is in the LICENSE.txt file included
 * with this distribution.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * wxWindows Library License for more details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "pf.h"
#include "vglutil.h"
#include "Timer.h"


#define BENCHTIME 2.0

#define BMPPAD(pitch) ((pitch+(sizeof(int)-1))&(~(sizeof(int)-1)))

#define _throw(m) {  \
	printf("\n   ERROR: %s\n", m);  retval=-1;  goto bailout;  \
}


double testTime=BENCHTIME;
int getSetRGB=0;


void initBuf(unsigned char *buf, int width, int pitch, int height, PF pf)
{
	int i, j;

	for(j=0; j<height; j++)
	{
		for(i=0; i<width; i++)
		{
			memset(&buf[j*pitch+i*pf.size], 0, pf.size);
			pf.setRGB(&buf[j*pitch+i*pf.size], (i*256/width)%256,
				(j*256/height)%256, (j*256/height+i*256/width)%256);
		}
	}
}


int cmpBuf(unsigned char *buf, int width, int pitch, int height, PF pf)
{
	int i, j, retval=1;

	for(j=0; j<height; j++)
	{
		for(i=0; i<width; i++)
		{
			int r, g, b;
			pf.getRGB(&buf[j*pitch+i*pf.size], &r, &g, &b);
			if(r!=(i*256/width)%256 || g!=(j*256/height)%256
				|| b!=(j*256/height+i*256/width)%256)
				retval=0;
		}
	}
	return retval;
}


int doTest(int width, int height, PF srcpf, PF dstpf)
{
	int retval=0, iter=0, srcPitch=BMPPAD(width*srcpf.size),
		dstPitch=BMPPAD(width*dstpf.size);
	unsigned char *srcBuf=NULL, *dstBuf=NULL, *srcPixel, *dstPixel;
	double tStart, elapsed;

	if((srcBuf=(unsigned char *)malloc(srcPitch*height))==NULL)
		_throw("Could not allocate memory");
	initBuf(srcBuf, width, srcPitch, height, srcpf);
	if((dstBuf=(unsigned char *)malloc(dstPitch*height))==NULL)
		_throw("Could not allocate memory");
	memset(dstBuf, 0, dstPitch*height);

	if(getSetRGB)
	{
		printf("%-4s --> %-4s (getRGB/setRGB):  ", srcpf.name, dstpf.name);
		tStart=getTime();
		iter=0;
		do
		{
			int h=height;
			unsigned char *srcRow=srcBuf, *dstRow=dstBuf;
			while(h--)
			{
				int w=width;
				srcPixel=srcRow;  dstPixel=dstRow;
				while(w--)
				{
					int r, g, b;
					srcpf.getRGB(srcPixel, &r, &g, &b);
					dstpf.setRGB(dstPixel, r, g, b);
					srcPixel+=srcpf.size;  dstPixel+=dstpf.size;
				}
				srcRow+=srcPitch;  dstRow+=dstPitch;
			}
			iter++;
		} while((elapsed=getTime()-tStart)<testTime);
	}
	else
	{
		printf("%-4s --> %-4s (pf_convert):  ", srcpf.name, dstpf.name);
		tStart=getTime();
		do
		{
			srcpf.convert(srcBuf, width, srcPitch, height, dstBuf, dstPitch, dstpf);
			iter++;
		} while((elapsed=getTime()-tStart)<testTime);
	}

	if(!cmpBuf(dstBuf, width, width*dstpf.size, height, dstpf))
	{
		printf("Pixel data is bogus\n");
		retval=-1;  goto bailout;
	}

	printf("%f Mpixels/sec\n",
		(double)(width*height)/1000000.*(double)iter/elapsed);

	bailout:
	if(srcBuf) free(srcBuf);
	if(dstBuf) free(dstBuf);
	return retval;
}


void usage(char **argv)
{
	fprintf(stderr, "\nUSAGE: %s [options]\n\n", argv[0]);
	fprintf(stderr, "Options:\n");
	fprintf(stderr, "-time <t> = Set benchmark time to <t> seconds (default: %.1f)\n",
		BENCHTIME);
	fprintf(stderr, "-getsetrgb = Use pixel format getRGB/setRGB methods for conversion\n\n");
	exit(1);
}


int main(int argc, char **argv)
{
	int retval=0, width=1024, height=768, srcFormat, dstFormat, i;

	if(argc>1) for(i=1; i<argc; i++)
	{
		if(!stricmp(argv[i], "-h") || !strcmp(argv[i], "-?")) usage(argv);
		else if(!stricmp(argv[i], "-time") && i<argc-1)
		{
			testTime=atof(argv[++i]);
			if(testTime<=0.0) usage(argv);
		}
		else if(!stricmp(argv[i], "-getsetrgb")) getSetRGB=1;
		else usage(argv);
	}

	for(srcFormat=0; srcFormat<PIXELFORMATS-1; srcFormat++)
	{
		PF srcpf=pf_get(srcFormat);
		for(dstFormat=0; dstFormat<PIXELFORMATS-1; dstFormat++)
		{
			PF dstpf=pf_get(dstFormat);
			if(doTest(width, height, srcpf, dstpf)==-1)
				goto bailout;
		}
		printf("\n");
	}

	bailout:
	return retval;
}