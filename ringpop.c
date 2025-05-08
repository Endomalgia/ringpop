
/* includes */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <argp.h>

/* configuration */
#include "config.h"

/* core includes */
#include "wri.h"

/* variables */

struct commandline_args {int verbose, quiet; };
static error_t parse_opt(int key, char* arg, struct argp_state* state) {
	struct commandline_args* arguments = state->input;
	switch(key) {
	case 'i':

    	break;
   	case 'o':
   		
   		break;
   	case 'a':
   		
   		break;
    case 'v':
      	arguments->verbose = 1;
      	break;
    case 'q':
      	arguments->quiet = 1;
      	break;
    case ARGP_KEY_ARG:
      if (state->arg_num >= 2)
        argp_usage(state);
    	break; // Break here?
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



  char* asset_paths[] = {"./1.txt", "./2.txt", "./3.txt", "./4.txt"};
  RIASSET* assets = malloc(4*sizeof(RIASSET));
  wriOpenAssets(asset_paths, assets, 4);

  printf("Opening encoder...\n");
  RIEncoder* enc = wriStartEncoder("./out.ri");

  printf("Appending assets...\n\n");
  wriAppendAssets(enc, &(assets[2]), 1);

  wriAppendAssets(enc, assets, 2);


  wriReadMaster(enc);

  struct stat info;
  fstat(enc->fptr, &info);
  printf("ENCODER LENGTH = %d(read) vs %d(actual)\n", enc->length, info.st_size);

  wriReadDAC(enc);

  printf("NUMBER OF ASSETS READ = %d\n", enc->na);
  for (int i=0; i<enc->na; i++) {
    printf("ASSET [%d]\n\tNAME: \"%.*s\"\n\tOFFSET: %ld\n\tLENGTH: %ld\n", i, 16, enc->assets[i].name, enc->assets[i].offset, enc->assets[i].length);
  }

  printf("Closing encoder...\n");
  wriCloseEncoder(enc);
  free(assets);


  printf("Reopening encoder by filepath...\n\n");
  RIEncoder* new_enc;
  new_enc = wriOpenEncoder("./out.ri", O_RDWR);

  printf("Appending file...\n\n");
  wriAppendAssets(new_enc, &(assets[3]), 1);

  char buf[512*4];
  printf("ENCODER LENGTH = %d\n", new_enc->length);
  printf("NUMBER OF ASSETS READ = %d\n", new_enc->na);
  for (int i=0; i<new_enc->na; i++) {
    printf("ASSET [%d]\n\tNAME: \"%.*s\"\n\tOFFSET: %ld\n\tLENGTH: %ld\n", i, 16, new_enc->assets[i].name, new_enc->assets[i].offset, new_enc->assets[i].length);
    lseek(new_enc->fptr, new_enc->assets[i].offset, SEEK_SET);
    read(new_enc->fptr, buf, new_enc->assets[i].length);
    printf("\tREAD : \"%.*s\"\n", new_enc->assets[i].length, buf);
  }

  printf("Reclosing encoder...\n");
  wriCloseEncoder(new_enc);

  printf("Deleting file...\n");
  remove("./out.ri");

	return 0;
}
