
/* includes */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <argp.h>
#include <sndfile.h>

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
char*   HEADER_FILE = NULL;

int     N_ASSETS = 0;

/* argp */
struct commandline_args {int verbose, do_append, do_create, do_header;
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
    case 'h':
      HEADER_FILE = arg;
      arguments->do_header = TRUE;
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
	struct commandline_args cmd_args = {FALSE,FALSE,FALSE,FALSE,NULL,NULL};

  // Prepare INPUT_FILES array
  INPUT_FILES = malloc(0);

	// Parse Command Arguments
	argp_parse(&argp, argc, argv, ARGP_IN_ORDER, 0, &cmd_args);


  // If no arguments were passed exit to 0
  if (cmd_args.do_create | cmd_args.do_append) {

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

    if (cmd_args.do_header) {

    }


    // Read auio data from the second asset
    SF_VIRTUAL_IO sf_vert = { (sf_count_t(*)(void*)) &_SNDFILE_LENGTH_CALLBACK, 
                              (sf_count_t(*)(sf_count_t, int, void*)) &_SNDFILE_SEEK_CALLBACK, 
                              (sf_count_t(*)(void*, sf_count_t, void*)) &_SNDFILE_READ_CALLBACK, 
                              (sf_count_t(*)(const void*, sf_count_t, void*)) &_SNDFILE_WRITE_CALLBACK, 
                              (sf_count_t(*)(void *)) &_SNDFILE_TELL_CALLBACK};
    SF_INFO* sf_info = malloc(sizeof(SF_INFO));


    enc->assets[1].encoder_fptr = enc->fptr;
    lseek(enc->fptr, enc->assets[1].offset, SEEK_SET);
    SNDFILE* sf = sf_open_virtual(&sf_vert, SFM_READ, sf_info, &(enc->assets[1]));
    
    sf_perror(sf); // Adding another asset breaks this :/ (no matter which end it is added to (dac fuckery most likely (during append or write)))
 
    printf("AUDIO FILE INFO:\n\tframes - %ld\n\trate - %d\n\tchannels - %d\n\tformat - 0x%x\n", sf_info->frames, sf_info->samplerate, sf_info->channels, sf_info->format);

    sf_close(sf);
    free(sf_info);


    
    // Read image data from the first asset
    int img_x, img_y, n_channels;

    lseek(enc->fptr, enc->assets[0].offset, SEEK_SET);
    stbi_io_callbacks stbi_vert = { (int(*)(void*,char*,int)) &_STBIMAGE_READ_CALLBACK,
                                    (void(*)(void*,int)) &_STBIMAGE_SKIP_CALLBACK,
                                    (int(*)(void*)) &_STBIMAGE_EOF_CALLBACK};
    unsigned char* img_dat = stbi_load_from_callbacks(&stbi_vert, &(enc->assets[0]), &img_x, &img_y, &n_channels, 0);

    printf("IMAGE FILE INFO:\n\twidth - %d\n\theight - %d\n\tchannels - %d\n",img_x, img_y, n_channels);

    stbi_image_free(img_dat);

    

    (cmd_args.verbose) ? printf("Closing encoder...\n"):0;
    wriCloseEncoder(enc);
  }

  free(INPUT_FILES);

	return 0;
}
