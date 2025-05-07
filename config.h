#ifndef RINGPOP_CONFIG_H
#define RINGPOP_CONFIG_H

/* ///////////////////////////// COMMAND LINE ////////////////////////////// */

const char* argp_program_bug_address             = "<address@mail.com>";
const char* argp_program_version                 = "ringpop v0.0.0a 'Abiu Attack'"; // https://simple.wikipedia.org/wiki/List_of_fruits (1)
static char argp_program_desc[]                  = "a.out -- Test test";
static char argp_program_args_desc[]             = "[ARGS]";
static struct argp_option argp_program_options[] = {
  {"input",     'i',  0,  0,  "Input files for addition to a ring"},
  {"output",    'o',  0,  0,  "Ring to output to"},
  {"append",    'a',  0,  0,  "Ring to append to"},
  {"list",      'l',  0,  0,  "List assets contained within the following ring file"},
  {"play",      'p',  0,  0,  "View assets in the given ring file"},
  {"verbose",   'v',  0,  0,  "Produce extended output"},
  {"quiet",     'q',  0,  0,  "Produce no output"},
  { 0 }};

/* //////////////////////////////// FORMAT ///////////////////////////////// */
/* 
	[Master RING chunk]
		MasterBlockID (4b)  | "RING" (0x52, 0x49, 0x4E, 0x47)
		FileSize	  (4b)  | Size of the file in bytes (unsigned long int)
		VersionString (16b) | Condensed version string (null terminated)
	[Data array chunk]
		DABlockID     (4b)  | "dac␣" (0x64, 0x61, 0x63, 0x20)
		NAssets		  (2b)	| Number of assets (unsigned short int)
		DataArrat     (26nb)| Array of asset file information
								AssetType (2b)	| Asset type
								AssetID   (16b) | Name of asset (null terminated)
								AssetLen  (4b)	| Length of the asset in bytes
								AssetLoc  (4b)	| Location of the asset in the file
	[Data chunk]
		DataBlockID	  (4b)	| "data" (0x52, 0x49, 0x4E, 0x47)
		AssetData	  (xnb)	| File data separated by something? idk yet tbh			
*/

/*
static const char FMT_MASTERBLOCKID[4]						= {0x52, 0x49, 0x4E, 0x47};	// 4b RING
static const char FMT_DATAARRAYBLOCKID[4]				  = {0x64, 0x61, 0x63, 0x20};	// 4b dac␣
static const char FMT_DATABLOCKID[4]							= {0x52, 0x49, 0x4E, 0x47};	// 4b data
static const char FMT_VERSIONSTRING[16]						= "v0.0.0a Abiu\0\0\0\0";   // 16b Is a /0 automatically added when strings are defined like this?
*/

#endif