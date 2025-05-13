#include "wri.h"

/* stb image implementation */

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

/* sik defines X3 */

#define MIN(i, j) (((i) < (j)) ? (i) : (j))
#define MAX(i, j) (((i) > (j)) ? (i) : (j))

/* these aren't really used (except the last 2) but theyr cool so i'm keeping them. bite me */

// https://stackoverflow.com/questions/109023/count-the-number-of-set-bits-in-a-32-bit-integer
int nSetBits(int i) {
	i = i - ((i >> 1) & 0x55555555);        		// add pairs of bits
    i = (i & 0x33333333) + ((i >> 2) & 0x33333333); // quads
    i = (i + (i >> 4)) & 0x0F0F0F0F;        		// groups of 8
    i *= 0x01010101;                        		// horizontal sum of bytes
    return  i >> 24;  
}

int nUsedBytes(int i) {
	if (i<=0)
		return 0;
	int o = 1;
	int t = i;
	while(t >>= 8)
		o++;
	return o;
}

// hex string to integer (with leading zeros and fixed length)
int xtoi(char* xbuf, int nelem) {
	if (nelem == 0)
		return 0;
	unsigned int o = 0;
	for (int i=0; i<nelem; i++) {
		o+=((unsigned char)xbuf[i] << (8*i));
	}
	return (int)o;
}

long int flen(int fd) {
	struct stat info;
	fstat(fd, &info);
	return (long)info.st_size;
}

RIEncoder* wriStartEncoder(char* ri_filepath) {
	RIEncoder* enc;

	// Check if the file exists
	if (access(ri_filepath, F_OK) == 0) {
		printf("[E] wriStartEncoder: File already exists [%s]\n\t%s\n", ri_filepath, strerror(errno));
		exit(0);
	}

	enc = malloc(sizeof(RIEncoder));
	enc->filename = ri_filepath;
	enc->assets = malloc(0);
	enc->fptr = open(ri_filepath, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
	enc->na = enc->length = 0;
	if (enc->fptr < 0) {
		printf("[E] wriStartEncoder: Unable to open file [%s]\n\t%s\n", ri_filepath, strerror(errno));
		exit(0);
	}

	// Write out empty data array chunk
	wriWriteDAC(enc);

	// Write the data header label thingie
	lseek(enc->fptr, FMT_SIZEOF_MASTERBLOCK + FMT_SIZEOF_DACHEADER + FMT_SIZEOF_DACASSET*enc->na, SEEK_SET);
	write(enc->fptr, FMT_DATABLOCKID, FMT_SIZEOF_DATAHEADER);

	// Write out the master ring chunk
	wriWriteMaster(enc);

	return enc;
}

RIEncoder* wriOpenEncoder(char* ri_filepath, int access_mode) {
	RIEncoder* enc;

	// Check if the file doesn't exist
	if (access(ri_filepath, F_OK) != 0) {
		printf("[E] wriOpenEncoder: File does not exist [%s]\n\t%s\n", ri_filepath, strerror(errno));
		exit(0);
	}

	enc = malloc(sizeof(RIEncoder));
	enc->filename = ri_filepath;
	enc->assets = malloc(0);
	enc->fptr = open(ri_filepath, access_mode, S_IRUSR | S_IWUSR);
	enc->na = enc->length = 0;
	if (enc->fptr < 0) {
		printf("[E] wriOpenEncoder: Unable to open file [%s]\n\t%s\n", ri_filepath, strerror(errno));
		exit(0);
	}

	wriReadMaster(enc);
	wriReadDAC(enc);

	return enc;
}

void wriCloseEncoder(RIEncoder* enc) {
	for (int i=0; i<enc->na; i++) {
		close(enc->assets[i].asset_fptr);
		free(enc->assets[i].name);
	}
	free(enc->assets);
	close(enc->fptr);
}

void wriWriteMaster(RIEncoder* enc) {
	enc->length = (long)MAX(flen(enc->fptr), FMT_SIZEOF_MASTERBLOCK);
	char buf[FMT_SIZEOF_MASTERBLOCK];

	memset(buf, 0, FMT_SIZEOF_MASTERBLOCK);
	memcpy(buf, 							FMT_MASTERBLOCKID, 	4);
	memcpy(buf+4, 							&(enc->length), 	4); 
	memcpy(buf+8, 							FMT_VERSIONSTRING, 	16);

	lseek(enc->fptr, 0, SEEK_SET);
	write(enc->fptr, buf,	FMT_SIZEOF_MASTERBLOCK);
}

void wriWriteDAC(RIEncoder* enc) {
	int dac_size = 6 + (FMT_SIZEOF_DACASSET * enc->na);
	char* buf = malloc(dac_size);

	memset(buf, 0, dac_size);

	memcpy(buf, 									FMT_DATAARRAYBLOCKID, 				4);
	memcpy(buf+4, 									&(enc->na), 						2);
	enc->length = FMT_SIZEOF_MASTERBLOCK + FMT_SIZEOF_DACHEADER + FMT_SIZEOF_DACASSET*enc->na + FMT_SIZEOF_DATAHEADER;
	for (int i=0;i<enc->na;i++) {
		int l = 6 + (FMT_SIZEOF_DACASSET * i);
		enc->assets[i].offset = enc->length;

		memcpy(buf+l,										&(enc->assets[i].type), 	2);
		memcpy(buf+l+2,										enc->assets[i].name, 		MIN(16, strlen(enc->assets[i].name))); // Remove ext and null
		memcpy(buf+l+18,									&(enc->assets[i].length), 	4);
		memcpy(buf+l+22,									&(enc->assets[i].offset), 	4);
		
		enc->length += enc->assets[i].length + FMT_SIZEOF_FILEDIVIDER;
	}

	lseek(enc->fptr, FMT_SIZEOF_MASTERBLOCK, SEEK_SET);
	write(enc->fptr, buf,	dac_size);
	free(buf);
}

void wriAppendAssets(RIEncoder* enc, RIASSET* assets, int n) {
	char content_buf[SIZEOF_READBUFFER];
	char size_buf[4];
	struct stat info;

	long int eof_data_offset = FMT_SIZEOF_DACASSET*n;
	long int new_eof = eof_data_offset + enc->length;
	enc->na += n;

	// Append each asset to the encoder
	enc->assets = realloc(enc->assets, enc->na * sizeof(RIASSET));
	for (int a=0; a<n; a++) {
		assets[a].encoder_fptr = enc->fptr;
		enc->assets[a+enc->na-n] = assets[a];
	}

	// Offset the current data in chunks of size SIZEOF_READBUFFER
	lseek(enc->fptr, 0, SEEK_END); // Number of blocks does not include any remainder
	long int data_start = FMT_SIZEOF_MASTERBLOCK + FMT_SIZEOF_DACHEADER + FMT_SIZEOF_DACASSET*(enc->na-n) + FMT_SIZEOF_DATAHEADER;
	long int data_len = enc->length - data_start;
	int n_blocks = (int)ceil((double)data_len / (double)SIZEOF_READBUFFER);
	for (int b=1; b<n_blocks+1;b++) {
		lseek(enc->fptr, data_start + (n_blocks-b)*SIZEOF_READBUFFER, SEEK_SET);
		read(enc->fptr, content_buf, SIZEOF_READBUFFER);
		lseek(enc->fptr, data_start + (n_blocks-b)*SIZEOF_READBUFFER + eof_data_offset, SEEK_SET);
		write(enc->fptr, content_buf, SIZEOF_READBUFFER);
	}

	// Write to the dac
	wriWriteDAC(enc);

	// Write in the new data
	lseek(enc->fptr, new_eof, SEEK_SET);
	for (int c=enc->na-n; c<enc->na; c++) {
		while(read(enc->assets[c].asset_fptr, content_buf, SIZEOF_READBUFFER)) {
			write(enc->fptr, content_buf, SIZEOF_READBUFFER);
		}
		lseek(enc->fptr, new_eof+enc->assets[c].length, SEEK_SET);
		write(enc->fptr, FMT_FILEDIVIDER, FMT_SIZEOF_FILEDIVIDER);
		new_eof += enc->assets[c].length + FMT_SIZEOF_FILEDIVIDER; // Could also get the file pointer at the end of this loop with ftell()
	}
	ftruncate(enc->fptr, new_eof); // Prevent any 'remainders' (shorter last blocks) from becoming an issue XP

	// Rewrite the data header label thingie
	lseek(enc->fptr, FMT_SIZEOF_MASTERBLOCK + FMT_SIZEOF_DACHEADER + FMT_SIZEOF_DACASSET*enc->na, SEEK_SET);
	write(enc->fptr, FMT_DATABLOCKID, FMT_SIZEOF_DATAHEADER);

	// Write master header (last as it contains the file size);
	wriWriteMaster(enc);
}

void wriReadMaster(RIEncoder* enc) {
	char buf[4];
	lseek(enc->fptr, 4, SEEK_SET); // countof(FMT_MASTERBLOCKID)?
	read(enc->fptr, buf, 4);
	enc->length = xtoi(buf, 4);
	if (enc->length != flen(enc->fptr)) {
		printf("[E] wriReadMaster: file appears corrupted (%ld (read length) != %ld (actual length) %d)\n\t%s\n", enc->length, flen(enc->fptr), enc->fptr, strerror(errno));
		exit(0);
	}
}

void wriReadDAC(RIEncoder* enc) {
	char data_buf[26];

	lseek(enc->fptr, FMT_SIZEOF_MASTERBLOCK + 4, SEEK_SET);
	read(enc->fptr, data_buf, 2);
	enc->na = xtoi(data_buf, 2);
	enc->assets = realloc(enc->assets, enc->na * sizeof(RIASSET));
	for (int i=0; i<enc->na; i++) {
		read(enc->fptr, data_buf, 26); // Read entire asset into buffer

		enc->assets[i].encoder_fptr = enc->fptr;
		enc->assets[i].asset_fptr = -1; // Not relevent during a read, do manually.
		enc->assets[i].type = xtoi(data_buf,2);

		enc->assets[i].name = malloc(16); // FIND A WAY AROUND THIS!!!!
		memcpy(enc->assets[i].name, data_buf+2, 16);

		enc->assets[i].length = xtoi(data_buf+18,4);
		enc->assets[i].offset = xtoi(data_buf+22,4);
	}
}

void wriListAssets(RIEncoder* enc) {
	char version_buf[16];
	lseek(enc->fptr, 8, SEEK_SET);
	read(enc->fptr, version_buf, 16);
	printf("VERSION: %.*s\t - %s (%ldb) - \t NUMBER OF ASSETS: %d\n", 16, version_buf, enc->filename, enc->length, enc->na);
	for (int i=0; i<enc->na; i++) {
		printf("\tASSET [%d] - \"%.*s\"\n\t\tTYPE: ", i, 16, enc->assets[i].name, enc->assets[i].type, enc->assets[i].length, enc->assets[i].offset);

		switch(enc->assets[i].type) {
			case RIASSET_TYPE_LIBSNDFILE:
				printf("LIBSNDFILE");
				break;
			case RIASSET_TYPE_STB_IMAGE:
				printf("STBI IMAGE");
				break;
			default:
				printf("UNKNOWN TYPE");
		}

		printf("\n\t\tLENGTH: %ld\n\t\tPOSITION IN FILE: %ld\n", enc->assets[i].length, enc->assets[i].offset);
	}
}

int wriAssetGetFD(RIEncoder* enc, char* name) {
	wriReadDAC(enc);
	for (int i=0; i<enc->na; i++) {
		if (strcmp(name, enc->assets[i].name) == 0) {
			lseek(enc->fptr, enc->assets[i].offset, SEEK_SET);
			return enc->fptr;
		}
	}
	printf("[E] wriAssetGetFD: Unable to locate asset [%s]\n", name);
	exit(0);
}

int wriAssetGetFDbyIndex(RIEncoder* enc, int index) {
	if (index >= enc->na) {
		printf("[E] wriAssetGetFDbyIndex: Index out of range [%d out of %d]\n", index,enc->na);
		exit(0);
	}
	lseek(enc->fptr, enc->assets[index].offset, SEEK_SET);
	return enc->fptr;
}

void wriOpenAssets(char** fp_array, RIASSET* ri_array, int n) {
	for (int i=0;i<n;i++) {
		if (access(fp_array[i], F_OK) != 0) {
			printf("[E] wriOpenAssets: Unable to open asset [%s]\n\t%s\n", fp_array[i], strerror(errno));
			exit(0);
		}
		ri_array[i].asset_fptr 	= open(fp_array[i], O_RDONLY, S_IRUSR);
		ri_array[i].length 		= flen(ri_array[i].asset_fptr);

		ri_array[i].name = malloc(16);
		memcpy(ri_array[i].name, fp_array[i], 16);

		ri_array[i].offset 	= 0;
		ri_array[i].type 	= 0;

		// Treat the actual file as an encoder with offset 0. Sort of hacky but it works
		ri_array[i].encoder_fptr = ri_array[i].asset_fptr;
		wriTypeAsset(&(ri_array[i]));
		ri_array[i].encoder_fptr = -1;
	}
}

void wriTypeAsset(RIASSET* asset) {
	if (isLibsndfileDecodable(asset)) {
		asset->type = RIASSET_TYPE_LIBSNDFILE;
	} else if (isStbiiDecodable(asset)) {
		asset->type = RIASSET_TYPE_STB_IMAGE;
	} else {
		asset->type = RIASSET_TYPE_UNKNOWN;
}} // Cleaning this up the way I know how (weird bit manipulation) would only obscure its function imo

void wriWriteToHeader(RIEncoder* enc) {
	
}

int isLibsndfileDecodable(RIASSET* asset) {
    SF_VIRTUAL_IO sf_vert = { (sf_count_t(*)(void*)) &_SNDFILE_LENGTH_CALLBACK, 
                              (sf_count_t(*)(sf_count_t, int, void*)) &_SNDFILE_SEEK_CALLBACK, 
                              (sf_count_t(*)(void*, sf_count_t, void*)) &_SNDFILE_READ_CALLBACK, 
                              (sf_count_t(*)(const void*, sf_count_t, void*)) &_SNDFILE_WRITE_CALLBACK, 
                              (sf_count_t(*)(void *)) &_SNDFILE_TELL_CALLBACK};
    SF_INFO* sf_info = malloc(sizeof(SF_INFO));

    int is_lsfd = 0;

    lseek(asset->encoder_fptr, asset->offset, SEEK_SET);
    SNDFILE* sf = sf_open_virtual(&sf_vert, SFM_READ, sf_info, asset);

    is_lsfd = !!sf;

    sf_close(sf);
    free(sf_info);
    return is_lsfd;
}

int isStbiiDecodable(RIASSET* asset) {
	int is_stbid = 0;
	int dat;

    lseek(asset->encoder_fptr, asset->offset, SEEK_SET);
    stbi_io_callbacks stbi_vert = { (int(*)(void*,char*,int)) &_STBIMAGE_READ_CALLBACK,
                                    (void(*)(void*,int)) &_STBIMAGE_SKIP_CALLBACK,
                                    (int(*)(void*)) &_STBIMAGE_EOF_CALLBACK};
    unsigned char* img_dat = stbi_load_from_callbacks(&stbi_vert, asset, &dat, &dat, &dat, 0);

    is_stbid = !!img_dat;

    stbi_image_free(img_dat);
    return is_stbid;
}

int _SNDFILE_LENGTH_CALLBACK(RIASSET* asset) {
	return asset->length;}

int _SNDFILE_SEEK_CALLBACK(int offset, int whence, RIASSET* asset) {
	switch (whence) {
		case SEEK_SET:
			return lseek(asset->encoder_fptr, offset + asset->offset, whence);
		case SEEK_CUR:
			return lseek(asset->encoder_fptr, offset, whence);
		case SEEK_END:
			return lseek(asset->encoder_fptr, offset + asset->offset + asset->length, whence);
		default:
			printf("[E] _SNDFILE_SEEK_CALLBACK: Undefined whence of [%d]\n", whence);
			exit(0);
	}};

int _SNDFILE_READ_CALLBACK(void* ptr, int count, RIASSET* asset) {
	return read(asset->encoder_fptr, ptr, count);}	// May cause errors in future as it doesnt return 0 on EOF, ignoring for now but ill bome back to it if things break;

int _SNDFILE_WRITE_CALLBACK(const void* ptr, int count, RIASSET* asset) {
	return write(asset->encoder_fptr, ptr, count);}

int _SNDFILE_TELL_CALLBACK(RIASSET* asset) {
	return lseek(asset->encoder_fptr, 0, SEEK_CUR) - asset->offset;}

int _STBIMAGE_READ_CALLBACK(RIASSET* asset, char* data, int size) {
	return read(asset->encoder_fptr, data, size);}

void _STBIMAGE_SKIP_CALLBACK(RIASSET* asset, int n) {
	lseek(asset->encoder_fptr, n, SEEK_CUR);}

int _STBIMAGE_EOF_CALLBACK(RIASSET* asset) {
	return (lseek(asset->encoder_fptr,0,SEEK_CUR) >= asset->length + asset->offset);} // Might be giving EOF like 1 byte too early. Eh, only one way to find out.
