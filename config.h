#ifndef RINGPOP_CONFIG_H
#define RINGPOP_CONFIG_H

/* ///////////////////////////// COMMAND LINE ////////////////////////////// */

const char* argp_program_bug_address             = "<address@mail.com>";
const char* argp_program_version                 = "baphomet v0.0.0a 'Bael'"; // Linear order from the lesser key of solomon
static char argp_program_desc[]                  = "a.out -- Test test";
static char argp_program_args_desc[]             = "[ARGS]";
static struct argp_option argp_program_options[] = {
  {"verbose",   'v',  0,  0,  "Produce extended output"},
  {"quiet",     'q',  0,  0,  "Produce no output"},
  { 0 }};

#endif