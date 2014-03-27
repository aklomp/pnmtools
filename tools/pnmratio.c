#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "../pnmreader/pnmreader.h"
#include "../pnmwriter/pnmwriter.h"

enum xaffinity { XLEFT, XCENTER, XRIGHT };
enum yaffinity { YTOP, YMIDDLE, YBOTTOM };

struct job {
	struct pnmreader *pr;
	struct pnmwriter *pw;
	enum xaffinity xaffinity;
	enum yaffinity yaffinity;
	int xratio;
	int yratio;
	unsigned int out_wd;
	unsigned int out_ht;
	unsigned int colstart;
	unsigned int rowstart;
	bool ratio_possible;
};

static int
gcd (int x, int y)
{
	// Greatest Commmon Divisor:
	int a = (x > y) ? x : y;
	int b = (x > y) ? y : x;
	int c;

	for (;;) {
		if ((c = a % b) == 0) {
			return b;
		}
		a = b;
		b = c;
	}
	return 1;
}

static bool
get_size (unsigned int width, unsigned int height, struct job *job)
{
	// Given the ratios in job->xratio, job->yratio, and the size of the
	// image, calculate a new width and height with exactly the given
	// ratio, and return it in job->out_wd and job->out_ht. Return true if
	// the ratio is attainable, false if it's impossible for the image
	// (say, 1000:1 on a 10x10 square).
	int height_divisions = height / job->yratio;
	int width_divisions = width / job->xratio;
	bool use_width = (height_divisions > width_divisions) ? 1 : 0;
	int divisions = (use_width) ? width_divisions : height_divisions;
	int i;

	// Choose the side that will result in the least number of divisions.
	// For each division, calculate the corresponding width/height, and
	// if it fits into the image, use that:
	for (i = divisions; i >= 0; i--) {
		if (use_width && job->yratio * divisions <= height) {
			break;
		}
		if (!use_width && job->xratio * divisions <= width) {
			break;
		}
	}
	if (i == 0) {
		return job->ratio_possible = false;
	}
	job->out_wd = job->xratio * i;
	job->out_ht = job->yratio * i;
	return job->ratio_possible = true;
}

static bool
got_format (enum pnm_format format, void *userdata)
{
	// Upgrade output type to binary in all cases:
	if (format == FORMAT_PBM_ASC) format = FORMAT_PBM_BIN;
	if (format == FORMAT_PGM_ASC) format = FORMAT_PGM_BIN;
	if (format == FORMAT_PPM_ASC) format = FORMAT_PPM_BIN;

	return pnmwriter_format(((struct job *)userdata)->pw, format);
}

static bool
got_geometry (unsigned int width, unsigned int height, void *userdata)
{
	struct job *job = userdata;

	if (get_size(width, height, job) == false) {
		return false;
	}
	job->colstart
		= (job->xaffinity == XLEFT) ? 0
		: (job->xaffinity == XCENTER) ? (width - job->out_wd) / 2
		: width - job->out_wd;

	job->rowstart
		= (job->yaffinity == YTOP) ? 0
		: (job->yaffinity == YMIDDLE) ? (height - job->out_ht) / 2
		: height - job->out_ht;

	return pnmwriter_width(job->pw, job->out_wd)
	    && pnmwriter_height(job->pw, job->out_ht);
}

static bool
got_maxval (unsigned int maxval, void *userdata)
{
	return pnmwriter_maxval(((struct job *)userdata)->pw, maxval);
}

static bool
got_pixel (unsigned int col, unsigned int row, unsigned int r, unsigned int g, unsigned int b, void *userdata)
{
	struct job *job = userdata;

	if (col < job->colstart || col >= job->colstart + job->out_wd) {
		return true;
	}
	if (row < job->rowstart || row >= job->rowstart + job->out_ht) {
		return true;
	}
	return pnmwriter_pixel(job->pw, r, g, b);
}

static void
usage (void)
{
	char msg[] =
		"pnmratio xratio yratio [--xleft|-xl] [--xcenter|-xc] [--xright|-xr]\n"
		"                       [--ytop|-yt] [--ymiddle|-ym] [--ybottom|-yb]\n"
		"\n"
		"Crop a PNM image to exactly the given ratio, by trimming pixels\n"
		"from the edges. The optional parameters specify where to anchor\n"
		"the cropped image; default is (center,center).\n"
		"\n";

	fputs(msg, stderr);
}

static bool
parse_args (int argc, char **argv, struct job *job)
{
	// Get arguments; at least the x and y ratio:
	if (argc < 3) {
		return false;
	}
	if ((job->xratio = atoi(argv[1])) <= 0) {
		return false;
	}
	if ((job->yratio = atoi(argv[2])) <= 0) {
		return false;
	}
	// No more args?
	if (argc == 3) {
		return true;
	}
	for (int i = 3; i < argc; i++)
	{
		// If argument does not start with -, quit:
		if (argv[i][0] != '-') {
			fprintf(stderr, "Unknown option: '%s'\n\n", argv[i]);
			return false;
		}
		// Long option?
		if (argv[i][1] == '-') {
			if (strcmp(&argv[i][2], "xleft") == 0) {
				job->xaffinity = XLEFT;
				continue;
			}
			if (strcmp(&argv[i][2], "xcenter") == 0) {
				job->xaffinity = XCENTER;
				continue;
			}
			if (strcmp(&argv[i][2], "xright") == 0) {
				job->xaffinity = XRIGHT;
				continue;
			}
			if (strcmp(&argv[i][2], "ytop") == 0) {
				job->yaffinity = YTOP;
				continue;
			}
			if (strcmp(&argv[i][2], "ymiddle") == 0) {
				job->yaffinity = YMIDDLE;
				continue;
			}
			if (strcmp(&argv[i][2], "ybottom") == 0) {
				job->yaffinity = YBOTTOM;
				continue;
			}
			fprintf(stderr, "Unknown option: '%s'\n\n", argv[i]);
			return false;
		}
		// Short option?
		if (strcmp(&argv[i][1], "xl") == 0) {
			job->xaffinity = XLEFT;
			continue;
		}
		if (strcmp(&argv[i][1], "xc") == 0) {
			job->xaffinity = XCENTER;
			continue;
		}
		if (strcmp(&argv[i][1], "xr") == 0) {
			job->xaffinity = XRIGHT;
			continue;
		}
		if (strcmp(&argv[i][1], "yt") == 0) {
			job->yaffinity = YTOP;
			continue;
		}
		if (strcmp(&argv[i][1], "ym") == 0) {
			job->yaffinity = YMIDDLE;
			continue;
		}
		if (strcmp(&argv[i][1], "yb") == 0) {
			job->yaffinity = YBOTTOM;
			continue;
		}
		fprintf(stderr, "Unknown option: '%s'\n\n", argv[i]);
		return false;
	}
	return true;
}

int
main (int argc, char **argv)
{
	int d;
	int ret = 1;
	char *buf;
	size_t nread;
	size_t bufsize = 1024 * 1024;
	struct job job = { .ratio_possible = true };

	if (parse_args(argc, argv, &job) == false) {
		usage();
		goto out0;
	}
	if ((buf = malloc(bufsize)) == NULL) {
		goto out0;
	}
	// Divide xratio and yratio by their greatest common divisor:
	if ((d = gcd(job.xratio, job.yratio)) > 1) {
		job.xratio /= d;
		job.yratio /= d;
	}
	if ((job.pw = pnmwriter_create(stdout)) == NULL) {
		fputs("could not create pnmwriter\n", stderr);
		goto out1;
	}
	if ((job.pr = pnmreader_create(got_format, got_geometry, got_maxval, got_pixel, &job)) == NULL) {
		fputs("could not create pnmreader\n", stderr);
		goto out2;
	}
	while ((nread = fread(buf, 1, bufsize, stdin)) > 0) {
		switch (pnmreader_feed(job.pr, buf, nread)) {
			case PNMREADER_ABORTED: fputs(job.ratio_possible ? "aborted\n" : "impossible ratio\n", stderr); break;
			case PNMREADER_INVALID_CHAR: fputs("invalid char\n", stderr); break;
			case PNMREADER_UNSUPPORTED: fputs("unsupported\n", stderr); break;
			case PNMREADER_NO_SIGNATURE: fputs("not a PNM file\n", stderr); break;
			case PNMREADER_FINISHED: ret = 0; break;
			case PNMREADER_FEED_ME: continue;
			default: fputs("Unknown error\n", stderr); break;
		}
		break;
	}
	pnmreader_destroy(job.pr);
out2:	pnmwriter_destroy(job.pw);
out1:	free(buf);
out0:	return ret;
}
