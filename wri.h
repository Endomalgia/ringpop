#ifndef RINGPOP_WRI_H
#define RINGPOP_WRI_H

/*
*	wri.h
*	Created: 2-5-25
*	Description: Scripts for writing .ri files
*/

/* includes */
#include <portaudio.h>
#include <sndfile.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <math.h>

//#include "config.h"

#define SIZEOF_READBUFFER 64 // Make this a number of bits so that a faster bitshift operation may be used

static const int FMT_SIZEOF_MASTERBLOCK						= 24;
static const int FMT_SIZEOF_DACHEADER						= 6;
static const int FMT_SIZEOF_DATAHEADER						= 4;
static const int FMT_SIZEOF_DACASSET						= 26;
static const int FMT_SIZEOF_FILEDIVIDER						= 16;

static const char FMT_MASTERBLOCKID[4]						= {0x52, 0x49, 0x4E, 0x47};	// 4b RING
static const char FMT_DATAARRAYBLOCKID[4]				  	= {0x64, 0x61, 0x63, 0x20};	// 4b dac␣
static const char FMT_DATABLOCKID[4]						= {0x64, 0x61, 0x74, 0x61};	// 4b data
static const char FMT_VERSIONSTRING[16]						= "v1.0.0 Açaí\0\0\0";   // 16b Is a /0 automatically added when strings are defined like this?

static const char FMT_FILEDIVIDER[16]						= "\0EOF SEPARATOR!\0";		// 16b

/* macros */
#define RIASSET_TYPE_OGG	= 0x1;

typedef struct {
	int		fptr;
	char*	name;
	long	length;
	long 	offset;
	int 	type;
} RIASSET;

/* types */
typedef struct {
	int   				fptr;
	unsigned long  int 	length;
	unsigned short int 	na;
	RIASSET*			assets;
	char* 				filename;
} RIEncoder;

/* RI Encoder Creation */
RIEncoder* wriStartEncoder(char* ri_filepath);					// Creating new *.ri
RIEncoder* wriOpenEncoder(char* ri_filepath, int access_mode);	// Writing to existing *.ri (O_RDONLY, O_WRONLY, O_RDWR)
void wriCloseEncoder(RIEncoder* enc);

/* RI Writing */
void wriWriteMaster(RIEncoder* enc);
void wriWriteDAC(RIEncoder* enc);
void wriAppendAssets(RIEncoder* enc, RIASSET* assets, int n);
//void wriRemoveAssetbyIndex(RIEncoder* enc, int index); // I'm not going to do this until I have to use it which will be never. Just make another one lol :3.

/* RI Reading */
void wriReadMaster(RIEncoder* enc);
void wriReadDAC(RIEncoder* enc);
void wriListAssets(RIEncoder* enc);

/* Asset Access */
int wriAssetGetFD(RIEncoder* enc, char* name);
int wriAssetGetFDbyIndex(RIEncoder* enc, int index);

/* Asset Array */
void wriOpenAssets(char** fp_array, RIASSET* ri_array, int n);

#endif