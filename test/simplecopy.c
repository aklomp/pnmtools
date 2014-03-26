#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include "../pnmreader/pnmreader.h"

static enum pnm_format format;

static bool
got_format (enum pnm_format f, void *data)
{
	switch (f) {
		case FORMAT_UNKNOWN:
		case FORMAT_PBM_ASC:
		case FORMAT_PBM_BIN: fputs("P1\n", stdout); format = FORMAT_PBM_ASC; break;
		case FORMAT_PGM_ASC:
		case FORMAT_PGM_BIN: fputs("P2\n", stdout); format = FORMAT_PGM_ASC; break;
		case FORMAT_PPM_ASC:
		case FORMAT_PPM_BIN: fputs("P3\n", stdout); format = FORMAT_PPM_ASC; break;
	}
	return true;
}

static bool
got_geometry (unsigned int width, unsigned int height, void *data)
{
	fprintf(stdout, "%d %d\n", width, height);
	return true;
}

static bool
got_maxval (unsigned int maxval, void *data)
{
	if (format != FORMAT_PBM_ASC) {
		fprintf(stdout, "%d\n", maxval);
	}
	return true;
}

static bool
got_pixel (unsigned int col, unsigned int row, unsigned int r, unsigned int g, unsigned int b, void *data)
{
	if (format == FORMAT_PPM_ASC) {
		fprintf(stdout, "%d %d %d ", r, g, b);
	}
	else {
		fprintf(stdout, "%d ", r);
	}
	return true;
}

int
main (int argc, char **argv)
{
	size_t nread;
	size_t bufsize = 10000;
	struct pnmreader *pr;

	// Optional parameter: size of the buffer to pass to pnmreader.
	if (argc == 2) {
		bufsize = atoi(argv[1]);
	}
	if (bufsize == 0) {
		bufsize = 10000;
	}
	char buf[bufsize];

	// Read from stdin, write to stdout. Simple pipe copy for test purposes.
	if ((pr = pnmreader_create(got_format, got_geometry, got_maxval, got_pixel, NULL)) == NULL) {
		return 1;
	}
	while ((nread = fread(buf, 1, bufsize, stdin)) > 0) {
		switch (pnmreader_feed(pr, buf, nread)) {
			case PNMREADER_ABORTED: fputs("aborted\n", stderr); break;
			case PNMREADER_INVALID_CHAR: fputs("invalid char\n", stderr); break;
			case PNMREADER_UNSUPPORTED: fputs("unsupported\n", stderr); break;
			case PNMREADER_FINISHED: break;
			case PNMREADER_FEED_ME: continue;
			default: pnmreader_destroy(pr); return 1;
		}
	}
	pnmreader_destroy(pr);
	return 0;
}
