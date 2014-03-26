#include <stdbool.h>
#include <stdlib.h>

#include "pnmreader.h"

enum state {
	STATE_FORMAT,
	STATE_WIDTH,
	STATE_HEIGHT,
	STATE_MAXVAL,
	STATE_ASCDATA_PBM,
	STATE_ASCDATA_PGM,
	STATE_ASCDATA_PPM,
	STATE_BINDATA_PBM,
	STATE_BINDATA_PGM,
	STATE_BINDATA_PPM,
	STATE_FINISHED
};

enum charclass {
	CHAR_NUMERIC,
	CHAR_COMMENT,
	CHAR_WHITESPACE,
	CHAR_INVALID
};

struct pnmreader
{
	bool (*got_format) (enum pnm_format, void *userdata);
	bool (*got_geometry) (unsigned int width, unsigned int height, void *userdata);
	bool (*got_maxval) (unsigned int maxval, void *userdata);
	bool (*got_pixel) (unsigned int col, unsigned int row, unsigned int r, unsigned int g, unsigned int b, void *userdata);
	void *userdata;

	enum charclass charclass;
	unsigned char *cur;
	unsigned char *buf;
	size_t bufsize;
	unsigned int asciinum;

	enum state state;
	int substate;

	unsigned int seek;
	unsigned int width;
	unsigned int height;
	unsigned int maxval;
	unsigned int r;
	unsigned int g;
	unsigned int b;
	unsigned int col;
	unsigned int row;
	enum pnm_format format;
};

static inline bool
increment_cur (struct pnmreader *const pr)
{
	// Increment buffer pointer.
	// Returns true if this lands us on a valid byte.
	// Returns false if this lands us past the end of the buffer.

	pr->cur++;
	return (pr->cur < (pr->buf + pr->bufsize));
}

static void
classify_char (struct pnmreader *const pr, bool is_binary)
{
	// If we're inside a comment, the only escape is end-of-line:
	if (pr->charclass == CHAR_COMMENT) {
		if (*pr->cur == '\n' || *pr->cur == '\r') {
			pr->charclass = CHAR_WHITESPACE;
		}
		return;
	}
	// According to `man pbm`, only these constitute whitespace:
	if (*pr->cur == ' ' || *pr->cur == '\t' || *pr->cur == '\n' || *pr->cur == '\r') {
		pr->charclass = CHAR_WHITESPACE;
		return;
	}
	// In binary mode, only acknowledge '0' and '1':
	if (is_binary && (*pr->cur == '0' || *pr->cur == '1')) {
		pr->charclass = CHAR_NUMERIC;
		return;
	}
	// In full mode, allow '0' through '9':
	if (!is_binary && (*pr->cur >= '0' && *pr->cur <= '9')) {
		pr->charclass = CHAR_NUMERIC;
		return;
	}
	if (*pr->cur == '#') {
		pr->charclass = CHAR_COMMENT;
		return;
	}
	pr->charclass = CHAR_INVALID;
}

static enum pnmreader_result
skip_until_numeric (struct pnmreader *const pr, bool is_binary)
{
	for (;;) {
		classify_char(pr, is_binary);
		if (pr->charclass == CHAR_NUMERIC) {
			return PNMREADER_SUCCESS;
		}
		if (pr->charclass == CHAR_INVALID) {
			return PNMREADER_INVALID_CHAR;
		}
		// Char class must be whitespace or comment:
		if (!increment_cur(pr)) {
			return PNMREADER_FEED_ME;
		}
	}
}

static enum pnmreader_result
skip_single_whitespace (struct pnmreader *const pr)
{
	classify_char(pr, false);
	if (pr->charclass != CHAR_WHITESPACE) {
		return PNMREADER_INVALID_CHAR;
	}
	if (!increment_cur(pr)) {
		return PNMREADER_FEED_ME;
	}
	return PNMREADER_SUCCESS;
}

static enum pnmreader_result
read_ascii_number (struct pnmreader *const pr, bool is_binary)
{
	for (;;) {
		classify_char(pr, is_binary);
		if (pr->charclass == CHAR_WHITESPACE) {
			return PNMREADER_SUCCESS;
		}
		if (pr->charclass == CHAR_COMMENT) {
			return PNMREADER_SUCCESS;
		}
		if (pr->charclass == CHAR_INVALID) {
			return PNMREADER_INVALID_CHAR;
		}
		if (pr->charclass == CHAR_NUMERIC) {
			if (is_binary) {
				pr->asciinum = (*pr->cur - '0');
				return (increment_cur(pr))
					? PNMREADER_SUCCESS
					: PNMREADER_FEED_ME;
			}
			pr->asciinum *= 10;
			pr->asciinum += (*pr->cur - '0');
		}
		if (!increment_cur(pr)) {
			return PNMREADER_FEED_ME;
		}
	}
}

static enum pnmreader_result
state_format (struct pnmreader *const pr)
{
	// Signature consists of three bytes:
	// Whitespace is fairly unambiguous:
	// 0: the letter 'P';
	// 1: a number '1'..'6';
	// 2: a separator (whitespace or comment char).

	switch (pr->substate)
	{
		case 0:	if (*pr->cur != 'P') {
				return PNMREADER_NO_SIGNATURE;
			}
			pr->substate = 1;
			if (!increment_cur(pr)) {
				return PNMREADER_FEED_ME;
			}

		case 1:	switch (*pr->cur) {
				case '1': pr->format = FORMAT_PBM_ASC; break;
				case '2': pr->format = FORMAT_PGM_ASC; break;
				case '3': pr->format = FORMAT_PPM_ASC; break;
				case '4': pr->format = FORMAT_PBM_BIN; break;
				case '5': pr->format = FORMAT_PGM_BIN; break;
				case '6': pr->format = FORMAT_PPM_BIN; break;
				default : return PNMREADER_NO_SIGNATURE;
			}
			pr->substate = 2;
			if (!increment_cur(pr)) {
				return PNMREADER_FEED_ME;
			}

		case 2:	classify_char(pr, false);
			if (pr->charclass != CHAR_COMMENT
			 && pr->charclass != CHAR_WHITESPACE) {
				return PNMREADER_NO_SIGNATURE;
			}
			pr->substate = 3;
			if (!increment_cur(pr)) {
				return PNMREADER_FEED_ME;
			}

		case 3:	break;
	}
	if (pr->got_format != NULL) {
		if (pr->got_format(pr->format, pr->userdata) == false) {
			return PNMREADER_ABORTED;
		}
	}
	pr->state = STATE_WIDTH;
	pr->substate = 0;
	return PNMREADER_SUCCESS;
}

static enum pnmreader_result
state_width (struct pnmreader *const pr)
{
	enum pnmreader_result res;

	switch (pr->substate)
	{
		case 0:	if ((res = skip_until_numeric(pr, false)) != PNMREADER_SUCCESS) {
				return res;
			}
			pr->asciinum = 0;
			pr->substate = 1;

		case 1:	if ((res = read_ascii_number(pr, false)) != PNMREADER_SUCCESS) {
				return res;
			}
			pr->width = pr->asciinum;
	}
	if (pr->width == 0) {
		return PNMREADER_UNSUPPORTED;
	}
	// pr->cur is the first non-numeric character after the width;
	// leave it there:
	pr->state = STATE_HEIGHT;
	pr->substate = 0;
	return PNMREADER_SUCCESS;
}

static enum pnmreader_result
state_height (struct pnmreader *const pr)
{
	enum pnmreader_result res;

	switch (pr->substate)
	{
		case 0:	if ((res = skip_until_numeric(pr, false)) != PNMREADER_SUCCESS) {
				return res;
			}
			pr->asciinum = 0;
			pr->substate = 1;

		case 1:	if ((res = read_ascii_number(pr, false)) != PNMREADER_SUCCESS) {
				return res;
			}
			pr->height = pr->asciinum;
	}
	if (pr->height == 0) {
		return PNMREADER_UNSUPPORTED;
	}
	if (pr->got_geometry != NULL) {
		if (pr->got_geometry(pr->width, pr->height, pr->userdata) == false) {
			return PNMREADER_ABORTED;
		}
	}
	pr->state = STATE_MAXVAL;
	pr->substate = 0;
	return PNMREADER_SUCCESS;
}

static enum pnmreader_result
state_maxval (struct pnmreader *const pr)
{
	enum pnmreader_result res;

	// The bitmap formats do not have an explicit maxval in the file,
	// which would be redundant, so just set it here and skip the read:
	if (pr->format == FORMAT_PBM_ASC || pr->format == FORMAT_PBM_BIN) {
		pr->maxval = 1;
		pr->substate = 2;
	}
	switch (pr->substate)
	{
		case 0:	if ((res = skip_until_numeric(pr, false)) != PNMREADER_SUCCESS) {
				return res;
			}
			pr->asciinum = 0;
			pr->substate = 1;

		case 1:	if ((res = read_ascii_number(pr, false)) != PNMREADER_SUCCESS) {
				return res;
			}
			pr->maxval = pr->asciinum;
			pr->substate = 2;

		case 2:	break;
	}
	if (pr->maxval > 65535) {
		return PNMREADER_UNSUPPORTED;
	}
	if (pr->got_maxval != NULL) {
		if (pr->got_maxval(pr->maxval, pr->userdata) == false) {
			return PNMREADER_ABORTED;
		}
	}
	switch (pr->format)
	{
		case FORMAT_UNKNOWN: return PNMREADER_UNSUPPORTED; // Placate compiler
		case FORMAT_PBM_ASC: pr->state = STATE_ASCDATA_PBM; break;
		case FORMAT_PGM_ASC: pr->state = STATE_ASCDATA_PGM; break;
		case FORMAT_PPM_ASC: pr->state = STATE_ASCDATA_PPM; break;
		case FORMAT_PBM_BIN: pr->state = STATE_BINDATA_PBM; break;
		case FORMAT_PGM_BIN: pr->state = STATE_BINDATA_PGM; break;
		case FORMAT_PPM_BIN: pr->state = STATE_BINDATA_PPM; break;
	}
	pr->substate = 0;
	return PNMREADER_SUCCESS;
}

static enum pnmreader_result
emit_pixel (struct pnmreader *const pr, unsigned int r, unsigned int g, unsigned int b)
{
	if (r > pr->maxval || g > pr->maxval || b > pr->maxval) {
		return PNMREADER_INVALID_CHAR;
	}
	if (pr->got_pixel != NULL) {
		if (pr->got_pixel(pr->col, pr->row, r, g, b, pr->userdata) == false) {
			return PNMREADER_ABORTED;
		}
	}
	pr->col++;
	if (pr->col == pr->width) {
		pr->col = 0;
		pr->row++;
	}
	if (pr->row == pr->height) {
		pr->state = STATE_FINISHED;
		return PNMREADER_FINISHED;
	}
	return PNMREADER_SUCCESS;
}

static enum pnmreader_result
state_ascdata_pbm (struct pnmreader *const pr)
{
	enum pnmreader_result res;
	enum pnmreader_result emit_res;

	switch (pr->substate)
	{
		for (;;)
		{
		case 0:	if ((res = skip_until_numeric(pr, true)) != PNMREADER_SUCCESS) {
				return res;
			}
			pr->asciinum = 99;
			pr->substate = 1;

		case 1:	res = read_ascii_number(pr, true);
			if (pr->asciinum <= 1) {
				// With PBM ascii, we can still have read a valid number (0 or 1)
				// while not being able to move on to the next digit. Try to detect
				// that case: if pr->asciinum has changed, assume a valid digit:
				if ((emit_res = emit_pixel(pr, pr->asciinum, pr->asciinum, pr->asciinum)) != PNMREADER_SUCCESS) {
					return emit_res;
				}
			}
			pr->substate = 0;
			// Do the extra fetch for read_ascii_number if requested:
			if (res != PNMREADER_SUCCESS) {
				return res;
			}
		}
	}
	// Not reached, placate compiler:
	__builtin_unreachable();
}

static enum pnmreader_result
state_ascdata_pgm (struct pnmreader *const pr)
{
	enum pnmreader_result res;

	switch (pr->substate)
	{
		for (;;)
		{
		case 0:	if ((res = skip_until_numeric(pr, false)) != PNMREADER_SUCCESS) {
				return res;
			}
			pr->asciinum = 0;
			pr->substate = 1;

		case 1:	if ((res = read_ascii_number(pr, false)) != PNMREADER_SUCCESS) {
				return res;
			}
			if ((res = emit_pixel(pr, pr->asciinum, pr->asciinum, pr->asciinum)) != PNMREADER_SUCCESS) {
				return res;
			}
			pr->substate = 0;
		}
	}
	// Not reached, placate compiler:
	__builtin_unreachable();
}

static enum pnmreader_result
state_ascdata_ppm (struct pnmreader *const pr)
{
	enum pnmreader_result res;

	for (;;) {
		switch (pr->substate)
		{
			case 0:
			case 2:
			case 4:	if ((res = skip_until_numeric(pr, false)) != PNMREADER_SUCCESS) {
					return res;
				}
				pr->asciinum = 0;
				pr->substate++;

			case 1:
			case 3:
			case 5:	if ((res = read_ascii_number(pr, false)) != PNMREADER_SUCCESS) {
					return res;
				}
				if (pr->substate == 1) {
					pr->r = pr->asciinum;
					pr->substate++;
					continue;
				}
				if (pr->substate == 3) {
					pr->g = pr->asciinum;
					pr->substate++;
					continue;
				}
				if ((res = emit_pixel(pr, pr->r, pr->g, pr->asciinum)) != PNMREADER_SUCCESS) {
					return res;
				}
				pr->substate = 0;
		}
	}
}

static enum pnmreader_result
state_bindata_pbm (struct pnmreader *const pr)
{
	enum pnmreader_result res;

	switch (pr->substate)
	{
		case 0:	pr->substate = 1;
			if ((res = skip_single_whitespace(pr)) != PNMREADER_SUCCESS) {
				return res;
			}

		for (;;)
		{
		case 1:	for (int i = 7; i >= 0; i--) {
				pr->r = (*pr->cur >> i) & 1;
				if ((res = emit_pixel(pr, pr->r, pr->r, pr->r)) != PNMREADER_SUCCESS) {
					return res;
				}
				// Vagaries of the format: the bits are packed per row,
				// and the last byte of the row may contain filler:
				if (pr->col == 0) {
					break;
				}
			}
			if (!increment_cur(pr)) {
				return PNMREADER_FEED_ME;
			}
		}
	}
	// Not reached, placate compiler:
	__builtin_unreachable();
}

static enum pnmreader_result
state_bindata_pgm (struct pnmreader *const pr)
{
	enum pnmreader_result res;

	switch (pr->substate)
	{
		case 0:	pr->substate = (pr->maxval > 255) ? 2 : 1;
			if ((res = skip_single_whitespace(pr)) != PNMREADER_SUCCESS) {
				return res;
			}
			if (pr->substate == 2) {
				goto state_2;
			}

		for (;;)
		{
		case 1:	// Single-byte PGM:
			pr->r = *pr->cur;
			if ((res = emit_pixel(pr, pr->r, pr->r, pr->r)) != PNMREADER_SUCCESS) {
				return res;
			}
			if (!increment_cur(pr)) {
				return PNMREADER_FEED_ME;
			}
		}

		for (;;)
		{
state_2:	case 2:	// Double-byte PGM:
			pr->r = *pr->cur;
			pr->substate = 3;
			if (!increment_cur(pr)) {
				return PNMREADER_FEED_ME;
			}

		case 3:	pr->r = ((pr->r << 8) | *pr->cur);
			if ((res = emit_pixel(pr, pr->r, pr->r, pr->r)) != PNMREADER_SUCCESS) {
				return res;
			}
			pr->substate = 2;
			if (!increment_cur(pr)) {
				return PNMREADER_FEED_ME;
			}
		}
	}
	// Not reached, placate compiler:
	__builtin_unreachable();
}

static enum pnmreader_result
state_bindata_ppm (struct pnmreader *const pr)
{
	enum pnmreader_result res;

	switch (pr->substate)
	{
		case 0:	pr->substate = (pr->maxval > 255) ? 4 : 1;
			if ((res = skip_single_whitespace(pr)) != PNMREADER_SUCCESS) {
				return res;
			}
			if (pr->substate == 4) {
				goto state_4;
			}

		for (;;)
		{
		case 1:	// Single-byte PPM:
			pr->r = *pr->cur;
			pr->substate = 2;
			if (!increment_cur(pr)) {
				return PNMREADER_FEED_ME;
			}

		case 2:	pr->g = *pr->cur;
			pr->substate = 3;
			if (!increment_cur(pr)) {
				return PNMREADER_FEED_ME;
			}

		case 3:	if ((res = emit_pixel(pr, pr->r, pr->g, *pr->cur)) != PNMREADER_SUCCESS) {
				return res;
			}
			pr->substate = 1;
			if (!increment_cur(pr)) {
				return PNMREADER_FEED_ME;
			}
		}

		for (;;)
		{
state_4:	case 4:	// Double-byte PPM:
			pr->r = *pr->cur;
			pr->substate = 5;
			if (!increment_cur(pr)) {
				return PNMREADER_FEED_ME;
			}

		case 5:	pr->r = ((pr->r << 8) | *pr->cur);
			pr->substate = 6;
			if (!increment_cur(pr)) {
				return PNMREADER_FEED_ME;
			}

		case 6:	pr->g = *pr->cur;
			pr->substate = 7;
			if (!increment_cur(pr)) {
				return PNMREADER_FEED_ME;
			}

		case 7:	pr->g = ((pr->g << 8) | *pr->cur);
			pr->substate = 8;
			if (!increment_cur(pr)) {
				return PNMREADER_FEED_ME;
			}

		case 8:	pr->b = *pr->cur;
			pr->substate = 9;
			if (!increment_cur(pr)) {
				return PNMREADER_FEED_ME;
			}

		case 9:	pr->b = ((pr->b << 8) | *pr->cur);
			if ((res = emit_pixel(pr, pr->r, pr->g, pr->b)) != PNMREADER_SUCCESS) {
				return res;
			}
			pr->substate = 4;
			if (!increment_cur(pr)) {
				return PNMREADER_FEED_ME;
			}
		}
	}
	// Not reached, placate compiler:
	__builtin_unreachable();
}

static enum pnmreader_result
state_finished (struct pnmreader *const pr)
{
	return PNMREADER_FINISHED;
}

struct pnmreader *
pnmreader_create (
	bool (*got_format) (enum pnm_format, void *userdata),
	bool (*got_geometry) (unsigned int width, unsigned int height, void *userdata),
	bool (*got_maxval) (unsigned int maxval, void *userdata),
	bool (*got_pixel) (unsigned int col, unsigned int row, unsigned int r, unsigned int g, unsigned int b, void *userdata),
	void *const userdata
)
{
	struct pnmreader *pr;

	if ((pr = malloc(sizeof(*pr))) == NULL) {
		return NULL;
	}
	pr->cur = NULL;
	pr->buf = NULL;
	pr->bufsize = 0;
	pr->col = 0;
	pr->row = 0;
	pr->charclass = CHAR_WHITESPACE;
	pr->state = STATE_FORMAT;
	pr->substate = 0;
	pr->seek = 0;
	pr->width = 0;
	pr->height = 0;

	pr->got_format = got_format;
	pr->got_geometry = got_geometry;
	pr->got_maxval = got_maxval;
	pr->got_pixel = got_pixel;
	pr->userdata = userdata;

	return pr;
}

void
pnmreader_destroy (struct pnmreader *pr)
{
	free(pr);
}

enum pnmreader_result
pnmreader_feed (struct pnmreader *pr, char *const data, size_t nbytes)
{
	// Jump table corresponding to the states:
	enum pnmreader_result (*state_jump_table[])(struct pnmreader *) = {
		state_format,
		state_width,
		state_height,
		state_maxval,
		state_ascdata_pbm,
		state_ascdata_pgm,
		state_ascdata_ppm,
		state_bindata_pbm,
		state_bindata_pgm,
		state_bindata_ppm,
		state_finished
	};
	if (pr == NULL) {
		return PNMREADER_ABORTED;
	}
	pr->buf = (unsigned char *)data;
	pr->cur = (unsigned char *)data;
	pr->bufsize = nbytes;

	for (;;)
	{
		// Execute handler for current state:
		enum pnmreader_result res = state_jump_table[pr->state](pr);

		// On success, continue to next state:
		if (res == PNMREADER_SUCCESS) {
			continue;
		}
		// Other status codes are passed on to caller:
		return res;
	}
}

bool
pnmreader_get_format (struct pnmreader *pr, enum pnm_format *format)
{
	if (pr == NULL) {
		return false;
	}
	if (format == NULL) {
		return false;
	}
	if (pr->state < STATE_WIDTH) {
		return false;
	}
	*format = pr->format;
	return true;
}

bool
pnmreader_get_geometry (struct pnmreader *pr, unsigned int *width, unsigned int *height)
{
	if (pr == NULL) {
		return false;
	}
	if (width == NULL) {
		return false;
	}
	if (height == NULL) {
		return false;
	}
	if (pr->state < STATE_MAXVAL) {
		return false;
	}
	*width = pr->width;
	*height = pr->height;
	return true;
}

bool
pnmreader_get_maxval (struct pnmreader *pr, unsigned int *maxval)
{
	if (pr == NULL) {
		return false;
	}
	if (maxval == NULL) {
		return false;
	}
	if (pr->state <= STATE_MAXVAL) {
		return false;
	}
	*maxval = pr->maxval;
	return true;
}
