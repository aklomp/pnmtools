#include <stdbool.h>
#include <stdio.h>

// Include header file for pnmreader:
#include "../pnmreader/pnmreader.h"

// This is a callback function that will be called by pnmreader_feed() when it
// has identified the image's width and height. If we return true, processing
// will continue. By returning false, we will abort the processing and stop
// further callbacks. pnmreader_feed() will return PNMREADER_ABORTED in the main
// loop below.
static bool
got_geometry (unsigned int width, unsigned int height, void *userdata)
{
	printf("%ux%u\n", width, height);
	return false;
}

int
main (int argc, char **argv)
{
	size_t nread, bufsize = 10000;
	struct pnmreader *pr;
	char buf[bufsize];

	// Create the pnmreader object, specify no callbacks except for got_geometry,
	// which will be called when the image width and height are found:
	if ((pr = pnmreader_create(NULL, got_geometry, NULL, NULL, NULL)) == NULL) {
		return 1;
	}
	// Read data from stdin:
	while ((nread = fread(buf, 1, bufsize, stdin)) > 0)
	{
		// Feed the data we read to pnmreader.
		// pnmreader_feed() interprets the buffer, possibly shoots off some
		// callbacks, and returns a status code that we interpret:
		switch (pnmreader_feed(pr, buf, nread))
		{
		case PNMREADER_FINISHED:	// file successfully read till end
		case PNMREADER_ABORTED:		// processing aborted by a callback
			break;

		// These are some error responses that we won't handle in any
		// specific depth here:
		case PNMREADER_UNSUPPORTED:
		case PNMREADER_INVALID_CHAR:
		case PNMREADER_NO_SIGNATURE:
			fputs("not a PNM file\n", stderr);
			break;

		// If the pnmreader asks to be fed, go back into the loop to
		// fetch more data:
		case PNMREADER_FEED_ME:
			continue;

		default: fputs("Unknown error\n", stderr);
			 break;
		}
		break;
	}
	pnmreader_destroy(pr);
	return 0;
}
