#include <stdbool.h>
#include <stdio.h>

#include "../pnmreader/pnmreader.h"
#include "../pnmwriter/pnmwriter.h"

static bool
got_format (enum pnm_format format, void *userdata)
{
	return pnmwriter_format(userdata, format);
}

static bool
got_geometry (unsigned int width, unsigned int height, void *userdata)
{
	return pnmwriter_width(userdata, width)
	    && pnmwriter_height(userdata, height);
}

static bool
got_maxval (unsigned int maxval, void *userdata)
{
	return pnmwriter_maxval(userdata, maxval);
}

static bool
got_pixel (unsigned int col, unsigned int row, unsigned int r, unsigned int g, unsigned int b, void *userdata)
{
	return pnmwriter_pixel(userdata, r, g, b);
}

int
main (int argc, char **argv)
{
	size_t nread;
	size_t bufsize = 10000;
	struct pnmreader *pr;
	struct pnmwriter *pw;
	char buf[bufsize];
	int ret = 1;

	if ((pw = pnmwriter_create(stdout)) == NULL) {
		return ret;
	}
	if ((pr = pnmreader_create(got_format, got_geometry, got_maxval, got_pixel, pw)) == NULL) {
		pnmwriter_destroy(pw);
		return ret;
	}
	while ((nread = fread(buf, 1, bufsize, stdin)) > 0) {
		switch (pnmreader_feed(pr, buf, nread)) {
			case PNMREADER_ABORTED: fputs("aborted\n", stderr); break;
			case PNMREADER_INVALID_CHAR: fputs("invalid char\n", stderr); break;
			case PNMREADER_UNSUPPORTED: fputs("unsupported\n", stderr); break;
			case PNMREADER_NO_SIGNATURE: fputs("not a PNM file\n", stderr); break;
			case PNMREADER_FINISHED: ret = 0; break;
			case PNMREADER_FEED_ME: continue;
			default: fputs("Unknown error\n", stderr); break;
		}
		break;
	}
	pnmreader_destroy(pr);
	pnmwriter_destroy(pw);
	return ret;
}
