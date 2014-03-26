#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include "pnmwriter.h"

enum state {
	STATE_FORMAT,
	STATE_WIDTH,
	STATE_HEIGHT,
	STATE_MAXVAL,
	STATE_DATA,
	STATE_FINISHED
};

struct pnmwriter {
	FILE *file;
	enum state state;
	enum pnm_format format;
	bool breakcols;
	bool hasnewline;
	unsigned int width;
	unsigned int height;
	unsigned int maxval;
	unsigned int col;
	unsigned int row;
	unsigned int linesize;
	unsigned char binvalue;
};

#define LINELEN	70

static inline int
numlen (unsigned int p)
{
	return (p < 10) ? 1
	     : (p < 100) ? 2
	     : (p < 1000) ? 3
	     : (p < 10000) ? 4
	     : 5;
}

static void
write_header (struct pnmwriter *const pw)
{
	if (pw == NULL) {
		return;
	}
	switch (pw->state) {
		case STATE_FORMAT:
			if (pw->format == FORMAT_UNKNOWN) {
				return;
			}
			if (fprintf(pw->file, "P%u\n", pw->format) < 0) {
				return;
			}
			pw->state = STATE_WIDTH;

		case STATE_WIDTH:
			if (pw->width == 0) {
				return;
			}
			if (fprintf(pw->file, "%u ", pw->width) < 0) {
				return;
			}
			pw->state = STATE_HEIGHT;

		case STATE_HEIGHT:
			if (pw->height == 0) {
				return;
			}
			if (fprintf(pw->file, "%u\n", pw->height) < 0) {
				return;
			}
			pw->state = STATE_MAXVAL;

		case STATE_MAXVAL:
			if (pw->maxval == 0) {
				return;
			}
			if (pw->format != FORMAT_PBM_ASC
			 && pw->format != FORMAT_PBM_BIN) {
				if (fprintf(pw->file, "%u\n", pw->maxval) < 0) {
					return;
				}
				// For ascii formats, decide whether to wrap
				// the lines based on the number of columns or
				// the max line length. If the number of
				// columns is small enough that a row of max
				// values fits within the LINELEN char line size,
				// wrap after each row, not the line length.
				if (pw->width < LINELEN) {
					if (pw->width * numlen(pw->maxval) + pw->width - 1 < LINELEN) {
						pw->breakcols = true;
					}
				}
			}
			pw->state = STATE_DATA;

		case STATE_DATA:
		case STATE_FINISHED:
			return;
	}
}

bool
pnmwriter_format (struct pnmwriter *const pw, enum pnm_format format)
{
	if (pw == NULL) {
		return false;
	}
	if (pw->state > STATE_FORMAT) {
		return false;
	}
	if (format == FORMAT_UNKNOWN) {
		return false;
	}
	if (pw->format != FORMAT_UNKNOWN && pw->format != format) {
		return false;
	}
	if (format == FORMAT_PBM_ASC
	 || format == FORMAT_PBM_BIN) {
		if (pw->maxval > 1) {
			return false;
		}
		pw->maxval = 1;
	}
	pw->format = format;
	write_header(pw);
	return true;
}

bool
pnmwriter_width (struct pnmwriter *const pw, unsigned int width)
{
	if (pw == NULL) {
		return false;
	}
	if (pw->state > STATE_WIDTH) {
		return false;
	}
	if (width == 0) {
		return false;
	}
	pw->width = width;
	write_header(pw);
	return true;
}

bool
pnmwriter_height (struct pnmwriter *const pw, unsigned int height)
{
	if (pw == NULL) {
		return false;
	}
	if (pw->state > STATE_HEIGHT) {
		return false;
	}
	if (height == 0) {
		return false;
	}
	pw->height = height;
	write_header(pw);
	return true;
}

bool
pnmwriter_maxval (struct pnmwriter *const pw, unsigned int maxval)
{
	if (pw == NULL) {
		return false;
	}
	if (pw->format == FORMAT_PBM_ASC
	 || pw->format == FORMAT_PBM_BIN) {
		if (maxval != 1) {
			return false;
		}
	}
	else if (pw->state > STATE_MAXVAL) {
		return false;
	}
	if (maxval == 0 || maxval > 65535) {
		return false;
	}
	pw->maxval = maxval;
	write_header(pw);
	return true;
}

static bool
write_ascii_value (struct pnmwriter *const pw, unsigned int p)
{
	int nchars = numlen(p);

	// Keep a line length limit of LINELEN characters:
	// Exactly enough space left:
	if (pw->linesize + nchars + 1 == LINELEN - 1) {
		if (fprintf(pw->file, " %u\n", p) < 0) {
			return false;
		}
		pw->hasnewline = true;
		pw->linesize = 0;
	}
	// Not enough space left, break line first:
	else if (pw->linesize + nchars + 1 >= LINELEN) {
		if (fprintf(pw->file, "\n%u", p) < 0) {
			return false;
		}
		pw->hasnewline = false;
		pw->linesize = nchars;
	}
	// We're breaking after full rows:
	else if (pw->breakcols && pw->col == pw->width - 1) {
		if (fprintf(pw->file, " %u\n", p) < 0) {
			return false;
		}
		pw->hasnewline = true;
		pw->linesize = 0;
	}
	// Start of new line, print without leading space:
	else if (pw->linesize == 0) {
		if (fprintf(pw->file, "%u", p) < 0) {
			return false;
		}
		pw->hasnewline = false;
		pw->linesize = nchars;
	}
	// Enough space, print normally with leading space:
	else {
		if (fprintf(pw->file, " %u", p) < 0) {
			return false;
		}
		pw->hasnewline = false;
		pw->linesize += nchars + 1;
	}
	return true;
}

static bool
write_binary_value (struct pnmwriter *const pw, unsigned int p)
{
	if (pw->maxval < 256) {
		fputc(p, pw->file);
		return (!feof(pw->file));
	}
	fputc((p >> 8) & 0xff, pw->file);
	if (feof(pw->file)) {
		return false;
	}
	fputc(p & 0xff, pw->file);
	return (!feof(pw->file));
}

bool
pnmwriter_pixel (struct pnmwriter *const pw, unsigned int r, unsigned int g, unsigned int b)
{
	if (pw == NULL) {
		return false;
	}
	if (pw->state != STATE_DATA) {
		return false;
	}
	if (r > pw->maxval) {
		return false;
	}
	if (g > pw->maxval) {
		return false;
	}
	if (b > pw->maxval) {
		return false;
	}
	switch (pw->format)
	{
		case FORMAT_PBM_ASC:
			if (pw->linesize == LINELEN - 1) {
				fputc('\n', pw->file);
				if (feof(pw->file)) {
					return false;
				}
				pw->hasnewline = true;
				pw->linesize = 0;
			}
			fputc((r == 1) ? '1' : '0', pw->file);
			if (feof(pw->file)) {
				return false;
			}
			if (pw->col == pw->width - 1) {
				fputc('\n', pw->file);
				if (feof(pw->file)) {
					return false;
				}
				pw->hasnewline = true;
				pw->linesize = 0;
			}
			else {
				pw->hasnewline = false;
				pw->linesize++;
			}
			break;

		case FORMAT_PGM_ASC:
			if (write_ascii_value(pw, r) == false) {
				return false;
			}
			break;

		case FORMAT_PPM_ASC:
			if (write_ascii_value(pw, r) == false
			 || write_ascii_value(pw, g) == false
			 || write_ascii_value(pw, b) == false) {
				return false;
			}
			break;

		case FORMAT_PBM_BIN:
			if (r == 1) {
				pw->binvalue |= (1 << (7 - (pw->col % 8)));
			}
			// Must collect 8 pixels to make output,
			// or this must be the last bit in the row:
			if (pw->col % 8 == 7 || pw->col == pw->width - 1) {
				fputc(pw->binvalue, pw->file);
				if (feof(pw->file)) {
					return false;
				}
				pw->binvalue = 0;
			}
			break;

		case FORMAT_PGM_BIN:
			if (write_binary_value(pw, r) == false) {
				return false;
			}
			break;

		case FORMAT_PPM_BIN:
			if (write_binary_value(pw, r) == false
			 || write_binary_value(pw, g) == false
			 || write_binary_value(pw, b) == false) {
				return false;
			}
			break;

		default: return false;
	}
	pw->col++;
	if (pw->col == pw->width) {
		pw->col = 0;
		pw->row++;
	}
	if (pw->row == pw->height) {
		pw->state = STATE_FINISHED;
		if (pw->format == FORMAT_PBM_ASC
		 || pw->format == FORMAT_PGM_ASC
		 || pw->format == FORMAT_PPM_ASC) {
			if (pw->hasnewline == false) {
				fputc('\n', pw->file);
				if (feof(pw->file)) {
					return false;
				}
			}
		}
	}
	return true;
}

struct pnmwriter *
pnmwriter_create (FILE *file)
{
	struct pnmwriter *pw;

	if ((pw = malloc(sizeof(*pw))) == NULL) {
		return NULL;
	}
	pw->file = file;
	pw->width = 0;
	pw->height = 0;
	pw->maxval = 0;
	pw->format = FORMAT_UNKNOWN;
	pw->state = STATE_FORMAT;
	pw->breakcols = false;
	pw->hasnewline = false;
	pw->col = 0;
	pw->row = 0;
	pw->linesize = 0;
	pw->binvalue = 0;
	return pw;
}

void
pnmwriter_destroy (struct pnmwriter *const pw)
{
	free(pw);
}
