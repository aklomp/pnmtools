# pnmtools

`pnmtools` is a little suite of programs to read and write images in the [netpbm](https://en.wikipedia.org/wiki/Netpbm_format) format.
There's a [stream reader](pnmreader) for reading PNM streams, and a [stream writer](pnmwriter) for writing them.
You can decode a stream on the fly as it comes in, and get callbacks immediately when something happens.
The [tools](tools) directory has some programs that use these routines.

Currently this package only supports PNM images, so no PAM.
It does, however, support all six PNM image formats, so binary/ascii monochrome/grayscale/rgb.

Keep in mind that the PNM image format is fairly loosely specified.
Does whitespace include a vertical tab?
`man pbm` claims it doesn't, but the online documentation claims it does.
This package has been tested for interoperability with the canonical `netpbm` package, and works as it should.
Tests are included in [test](test).

[![Build Status](https://travis-ci.org/aklomp/pnmtools.svg)](https://travis-ci.org/aklomp/pnmtools)

## pnmreader

`pnmreader` is a stream reader for reading PNM images.
This entails that you don't need to pass a whole complete file to get decoded output, but that any amount of data you have, from a single byte on, can be fed to the reader for inspection and processing.
Every time the reader finds something interesting in the input stream, such as the image geometry or a pixel value, it calls a callback function supplied by you.
The callbacks receive the data for any processing you care to do, plus a user-defined data pointer for any state you might want to pass in.

### pnmreader_create

To create a `pnmreader` object, call:

```c
struct pnmreader *
pnmreader_create
(
	// Called when the PNM format header has been decoded. Skipped when NULL.
	bool (*got_format) (enum pnm_format, void *userdata),

	// Called when the width and height have been decoded. Skipped when NULL.
	bool (*got_geometry) (unsigned int width, unsigned int height, void *userdata),

	// Called when the PNM max value has been determined. Skipped when NULL.
	bool (*got_maxval) (unsigned int maxval, void *userdata),

	// Called when a pixel has been read. If the image format is grayscale or
	// monochrome, r, g, and b will all have the same value. Skipped when NULL.
	bool (*got_pixel) (unsigned int col, unsigned int row, unsigned int r, unsigned int g, unsigned int b, void *userdata),

	// The user-supplied pointer to return to the user during a callback:
	void *const userdata
);
```

Each of these callback pointers can be `NULL` to do nothing.
Callbacks return a boolean.
If a callback returns `true`, processing will continue, if it returns `false`, processing will abort.
On `false`, your `pnmreader_feed` call will return with `PNMREADER_ABORTED` so that you can kill your main loop.

It's up to your callbacks to store or take action on the image data.
The pnmreader just decodes the stream and notifies you of what it found.

### pnmreader_destroy

To destroy a `pnmreader` object, call:

```c
void pnmreader_destroy (struct pnmreader *);
```

### pnmreader_feed

This is the "feeder" routine for pnmreader, the routine that you submit bytes to as they come in.
This routine will inspect the bytes and call your callbacks for you.

```c
enum pnmreader_result pnmreader_feed (struct pnmreader *, char *const data, size_t nbytes);
```

The return value is one of these enum constants:

* `PNMREADER_FEED_ME`: finished processing the buffer, submit more bytes;
* `PNMREADER_INVALID_CHAR`: invalid character in input;
* `PNMREADER_NO_SIGNATURE`: invalid PNM signature;
* `PNMREADER_UNSUPPORTED`: feature not supported (too high maxval, etc);
* `PNMREADER_FINISHED`: all done, image has been completely decoded;
* `PNMREADER_ABORTED`: one of your callbacks asked to abort processing.

### pnmreader_get_format

Retrieves the format code from a pnmreader object.
Returns false if the argument(s) are invalid or the format code has not been read.
Returns true on success, and writes the format code to the second argument.

```c
bool pnmreader_get_format (struct pnmreader *, enum pnm_format *);
```

The formats are:

* `FORMAT_UNKNOWN`
* `FORMAT_PBM_ASC`
* `FORMAT_PGM_ASC`
* `FORMAT_PPM_ASC`
* `FORMAT_PBM_BIN`
* `FORMAT_PGM_BIN`
* `FORMAT_PPM_BIN`

### pnmreader_get_geometry

Retrieve the geometry from the pnmreader object.
Returns false if the argument(s) are invalid or the geometry has not yet been established.
Returns true on success, and writes the geometry to the second and third arguments.

```c
bool pnmreader_get_geometry (struct pnmreader *, unsigned int *width, unsigned int *height);
```

### pnmreader_get_maxval

Retrieve the max value from the pnmreader object.
Returns false if the argument(s) are invalid or the max value has not been read.
Returns true on success, and writes the max value to the second argument.

```c
bool pnmreader_get_maxval (struct pnmreader *, unsigned int *maxval);
```

### Example

Here's the source of `imgsize.c` from the `test` directory as a short example of how it works.
It's a small demo program that prints the width and height of an image piped to its standard input:

```c
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
```

## pnmwriter

TODO

## License

`pnmtools` is licensed under the BSD 3-clause license.
