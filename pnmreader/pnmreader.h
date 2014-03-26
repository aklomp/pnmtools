#ifndef PNMREADER_H
#define PNMREADER_H

// The main structure, kept private:
struct pnmreader;

enum pnmreader_result
{
	PNMREADER_SUCCESS,
	PNMREADER_FEED_ME,
	PNMREADER_INVALID_CHAR,
	PNMREADER_NO_SIGNATURE,
	PNMREADER_UNSUPPORTED,
	PNMREADER_FINISHED,
	PNMREADER_ABORTED
};

#ifndef PNM_FORMAT
#define PNM_FORMAT
enum pnm_format
{
	FORMAT_UNKNOWN,
	FORMAT_PBM_ASC,
	FORMAT_PGM_ASC,
	FORMAT_PPM_ASC,
	FORMAT_PBM_BIN,
	FORMAT_PGM_BIN,
	FORMAT_PPM_BIN
};
#endif

// Create a new pnmreader struct.
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

// Destroy the pnmreader struct:
void pnmreader_destroy (struct pnmreader *);

// Push bytes into the pnmreader.
// The pnmreader will call your callbacks when it decodes appropriate data.
// Returns a return code from the pnmreader_result enum.
enum pnmreader_result
pnmreader_feed (struct pnmreader *, char *const data, size_t nbytes);

// Retrieve the format code from the pnmreader object.
// Returns false if the argument(s) are invalid or the format code has not been read.
// Returns true on success, and writes the format code to the second argument.
bool pnmreader_get_format (struct pnmreader *, enum pnm_format *);

// Retrieve the geometry from the pnmreader object.
// Returns false if the argument(s) are invalid or the geometry has not been established.
// Returns true on success, and writes the geometry to the second and third arguments.
bool pnmreader_get_geometry (struct pnmreader *, unsigned int *width, unsigned int *height);

// Retrieve the max value from the pnmreader object.
// Returns false if the argument(s) are invalid or the max value has not been read.
// Returns true on success, and writes the max value to the second argument.
bool pnmreader_get_maxval (struct pnmreader *, unsigned int *maxval);

#endif
