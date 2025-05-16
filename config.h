#ifndef RINGPOP_CONFIG_H
#define RINGPOP_CONFIG_H

/* ///////////////////////////// COMMAND LINE ////////////////////////////// */

const char* argp_program_bug_address             = "<nowhere@seethe.com>";
const char* argp_program_version                 = "ringpop v1.0.1 'Açaí Aggravation'"; // https://simple.wikipedia.org/wiki/List_of_fruits (1)
static char argp_program_desc[]                  = "a.out -- Test test";
static char argp_program_args_desc[]             = "[ARGS]";
static struct argp_option argp_program_options[] = {
  {"info",      'i',  "FILE",   0,  "Read the contents of a particular ring file"},
  {"output",    'o',  "FILE.ri",0,  "Ring to output to"},
  {"append",    'a',  "FILE.ri",0,  "Ring to append to"},
  {"files",     'f',  "FILE",   0,  "Assets to append to the ring (each file needs its own -f)"},
  {"header",    'h',  "FILE.ri",0,  "Generate a header file from a RIng (each *.ri needs its own -h)"},
  {"play",      'p',  0,        0,  "View assets in the given ring file"},
  {"verbose",   'v',  0,        0,  "Produce extended output"},
  { 0 }};

#define	DEFAULT_OUTPUT_FILENAME "out.ri";
#define	DEFAULT_HEADER_FILENAME "assets.h";



#endif