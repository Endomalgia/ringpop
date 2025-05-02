
/* includes */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <argp.h>

/* core includes */


/* configuration */
#include "config.h"

struct commandline_args {int verbose, quiet; };
static error_t parse_opt(int key, char* arg, struct argp_state* state) {
	struct commandline_args* arguments = state->input;
	switch(key) {
    case 'v':
      arguments->verbose = 1;
      break;
    case 'q':
      arguments->quiet = 1;
      break;
    case ARGP_KEY_ARG:
      if (state->arg_num >= 2)
        argp_usage(state);
    case ARGP_KEY_END:
      if (state->arg_num >= 2)
        argp_usage(state);
      break;
    default:
      return ARGP_ERR_UNKNOWN;
  }
  return 0;
}


struct argp argp = { argp_program_options, parse_opt, argp_program_args_desc, argp_program_desc };
int main(int argc, char* argv[]) {
	struct commandline_args cmd_args = {.verbose = 0, .quiet = 0};

	// Parse Command Arguments
	argp_parse(&argp, argc, argv, 0, 0, &cmd_args);
	(cmd_args.verbose) ? fprintf(stderr,"VERBOSE ENABLED:\n"):0;

	return 0;
}
