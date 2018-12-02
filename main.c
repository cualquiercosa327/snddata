#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#include <direct.h>
#else
#include <sys/stat.h>
#endif

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#define IOERR "Error: Failed to open \"%s\" for %s.\n"

typedef struct Header {
	char Signature[8];
	uint32_t FileCount;
	uint32_t Size;
	char Padding[16];
} Header;

typedef struct FileDesc {
	char Filename[32];
	uint32_t Offset;
	uint32_t Size;
	char Padding[24];
} FileDesc;

uint32_t le(uint32_t be) {
	return (be & 0xff) << 24 | (be & 0xff00) << 8 | (be & 0xff0000) >> 8 | (be & 0xff000000) >> 24;
}

int main(int argc, char **argv) {
	if (argc != 2) {
		fprintf(stderr, "Usage: %s snddata.obj\n", argv[0]);
		return 1;
	}

	FILE *input;

	if (!(input = fopen(argv[1], "rb"))) {
		fprintf(stderr, IOERR, argv[1], "reading");
		return 1;
	}

	char *dirName = strtok(argv[1], "."),
		 *name    = malloc(0x100);

#ifdef _WIN32
	if (_mkdir(dirName)) {
#else
	if (mkdir(dirName, 0777)) {
#endif
		fputs("Error: Failed to create directory.", stderr);
		return 1;
	}

	Header hdr;
	fread(&hdr, sizeof(hdr), 1, input);

	if (strcmp(hdr.Signature, "SNDFILE")) {
		fputs("Error: Not a SNDFILE.", stderr);
		return 1;
	}

	hdr.FileCount = le(hdr.FileCount);

	FileDesc *fd = calloc(hdr.FileCount, sizeof*fd);
	fread(fd, sizeof(FileDesc), hdr.FileCount, input);

	for (size_t i = 0; i < hdr.FileCount; i++) {
		sprintf(name, "%s/%s", dirName, fd[i].Filename);

		printf("Saving %s...\n", fd[i].Filename);

		char *buf = malloc(le(fd[i].Size));
		FILE *outf;

		if (!(outf = fopen(name, "wb"))) {
			fprintf(stderr, IOERR, fd[i].Filename, "writing");
			return 1;
		}

		fseek(input, le(fd[i].Offset), 0);
		fread(buf, 1, le(fd[i].Size), input);
		fwrite(buf, 1, le(fd[i].Size), outf);
		fclose(outf);
	}

	puts("\nDone!");
	fclose(input);

	return 0;
}
