#include "wri.h"

#define MIN(i, j) (((i) < (j)) ? (i) : (j))
#define MAX(i, j) (((i) > (j)) ? (i) : (j))

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
	while(t >>= 0x10)
		o++;
	return o;
}

// hex string to integer (with leading zeros and fixed length)
int xtoi(char* xbuf, int nelem) {
	if (nelem == 0) [[unlikely]]
		return 0;
	unsigned int o = 0;
	for (int i=0; i<nelem; i++) {
		o+=((unsigned char)xbuf[i] << (8*(nelem-i-1)));
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
		close(enc->assets[i].fptr);
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
	memcpy(buf+8-nUsedBytes(enc->length), 	&(enc->length), 	4);
	memcpy(buf+8, 							FMT_VERSIONSTRING, 	16);

	lseek(enc->fptr, 0, SEEK_SET);
	write(enc->fptr, buf,	FMT_SIZEOF_MASTERBLOCK);
}

void wriWriteDAC(RIEncoder* enc) {
	int dac_size = 6 + (FMT_SIZEOF_DACASSET * enc->na);
	char* buf = malloc(dac_size);

	memset(buf, 0, dac_size);

	memcpy(buf, 									FMT_DATAARRAYBLOCKID, 				4);
	memcpy(buf+6-nUsedBytes(enc->na), 				&(enc->na), 						2);
	enc->length = FMT_SIZEOF_MASTERBLOCK + FMT_SIZEOF_DACHEADER + FMT_SIZEOF_DACASSET*enc->na + FMT_SIZEOF_DATAHEADER;
	for (int i=0;i<enc->na;i++) {
		int l = 6 + (FMT_SIZEOF_DACASSET * i);
		enc->assets[i].offset = enc->length;

		memcpy(buf+l+2-nUsedBytes(enc->assets[i].type),		&(enc->assets[i].type), 	2);
		memcpy(buf+l+2,										enc->assets[i].name, 		MIN(16, strlen(enc->assets[i].name))); // Remove ext and null
		memcpy(buf+l+22-nUsedBytes(enc->assets[i].length),	&(enc->assets[i].length), 	4);
		memcpy(buf+l+FMT_SIZEOF_DACASSET-nUsedBytes(enc->assets[i].offset),	&(enc->assets[i].offset), 	4);
		
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
		while(read(enc->assets[c].fptr, content_buf, SIZEOF_READBUFFER)) {
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
		printf("[E] wriReadMaster: file appears corrupted (fd = %d)\n\t%s\n", enc->fptr, strerror(errno));
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

		enc->assets[i].fptr = -1; // Not relevent during a read, do manually.
		enc->assets[i].type = xtoi(data_buf,2);

		enc->assets[i].name = malloc(16); // FIND A WAY AROUND THIS!!!!
		memcpy(enc->assets[i].name, data_buf+2, 16);

		enc->assets[i].length = xtoi(data_buf+18,4);
		enc->assets[i].offset = xtoi(data_buf+22,4);
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
		ri_array[i].fptr 	= open(fp_array[i], O_RDONLY, S_IRUSR);
		ri_array[i].length 	= flen(ri_array[i].fptr);

		ri_array[i].name = malloc(16);
		memcpy(ri_array[i].name, fp_array[i], 16);
		//ri_array[i].name	= fp_array[i];
		ri_array[i].offset 	= -1;
		ri_array[i].type 	= 0; // Switch over extensions eventually
	}
}