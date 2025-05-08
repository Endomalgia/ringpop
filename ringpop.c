
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

/* defines */
#define TRUE 1
#define FALSE 0

/* variables */
char**  INPUT_FILES = NULL;
char*   OUTPUT_FILE = DEFAULT_OUTPUT_FILENAME;

int     N_ASSETS = 0;

/* argp */
struct commandline_args {int verbose, do_append, do_create;
                         char* out_file;
                         char** inputs;};
static error_t parse_opt(int key, char* arg, struct argp_state* state) {
	struct commandline_args* arguments = state->input;
	switch(key) {
	case 'i':

      // Print the file data and exit
      RIEncoder* enc;
      enc = wriOpenEncoder(arg, O_RDONLY);
      wriListAssets(enc);
      wriCloseEncoder(enc);
      exit(0);

    	break;
   	case 'o':
      if (arguments->do_append) {
        printf("[E] Invalid combination of arguments (-a with -o). Terminating program\n");
        exit(0);
      }

      OUTPUT_FILE = arg;
   		
      arguments->do_create = TRUE;
   		break;
   	case 'a':
   		if (arguments->do_create) {
        printf("[E] Invalid combination of arguments (-o with -a). Terminating program\n");
        exit(0);
      }

      OUTPUT_FILE = arg;
      
      arguments->do_append = TRUE;
   		break;
    case 'f':
      N_ASSETS++;
      INPUT_FILES = realloc(INPUT_FILES, sizeof(char*) * N_ASSETS);

      INPUT_FILES[N_ASSETS-1] = arg;
      break;
    case 'v':
      arguments->verbose = TRUE;
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
	struct commandline_args cmd_args = {FALSE,FALSE,FALSE,NULL,NULL};

  // Prepare INPUT_FILES array
  INPUT_FILES = malloc(0);

	// Parse Command Arguments
	argp_parse(&argp, argc, argv, ARGP_IN_ORDER, 0, &cmd_args);

  // If no arguments were passed exit to 0
  if (!(cmd_args.do_create | cmd_args.do_append)) {
    free(INPUT_FILES);
    return 0;
  }

  if (cmd_args.verbose) {
    printf("NAME OF OUTPUT FILE %s\n", OUTPUT_FILE);
    for (int i=0; i<N_ASSETS; i++) {
      printf("\tADDING ASSET [%i] - %s\n", i, INPUT_FILES[i]);
    }
  }

  (cmd_args.verbose) ? printf("Opening Assets...\n"):0;
  RIASSET* assets = malloc(N_ASSETS * sizeof(RIASSET));
  wriOpenAssets(INPUT_FILES, assets, N_ASSETS);

  (cmd_args.verbose) ? printf("Opening Encoder off of %s in O_RDWR...\n", OUTPUT_FILE):0;
  RIEncoder* enc = (cmd_args.do_create) ? wriStartEncoder(OUTPUT_FILE) : wriOpenEncoder(OUTPUT_FILE, O_RDWR);

  (cmd_args.verbose) ? printf("Appending [%d] assets to file...\n", N_ASSETS):0;
  wriAppendAssets(enc, assets, N_ASSETS);

  if (cmd_args.verbose) {
    wriListAssets(enc);
  }

  (cmd_args.verbose) ? printf("Closing encoder...\n"):0;
  wriCloseEncoder(enc);


  free(INPUT_FILES);
  
  /*
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

  wriListAssets(new_enc);

  printf("Reclosing encoder...\n");
  //wriCloseEncoder(new_enc);

  printf("Deleting file...\n");
  //remove("./out.ri");
  */

	return 0;
}
