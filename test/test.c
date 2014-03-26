#include <stdbool.h>
#include <stdio.h>

#include "../pnmreader/pnmreader.h"

struct test
{
	char *image;
	size_t nbytes;
	char *name;

	enum pnm_format format;
	unsigned int width;
	unsigned int height;
	unsigned int maxval;
	unsigned int *pixels;
	enum pnmreader_result result;
};

static bool
got_format (enum pnm_format format, void *data)
{
	struct test *t = data;
	if (format != t->format) {
		printf("Fail: %s: format: expected %d, got %d\n", t->name, t->format, format);
	}
	return true;
}

static bool
got_geometry (unsigned int width, unsigned int height, void *data)
{
	struct test *t = data;
	if (width != t->width) {
		printf("Fail: %s: width: expected %d, got %d\n", t->name, t->width, width);
	}
	if (height != t->height) {
		printf("Fail: %s: height: expected %d, got %d\n", t->name, t->height, height);
	}
	return true;
}

static bool
got_maxval (unsigned int maxval, void *data)
{
	struct test *t = data;
	if (maxval != t->maxval) {
		printf("Fail: %s: maxval: expected %d, got %d\n", t->name, t->maxval, maxval);
	}
	return true;
}

static bool
got_pixel (unsigned int col, unsigned int row, unsigned int r, unsigned int g, unsigned int b, void *data)
{
	struct test *t = data;
	if (r != t->pixels[row * t->width + col]) {
		printf("Fail: %s: pixels: at (%u,%u): expected %u, got %u\n", t->name, col, row, t->pixels[row * t->width + col], r);
	}
	return true;
}

static void
run_test (struct test *test)
{
	struct pnmreader *pr;
	enum pnmreader_result res;

	if ((pr = pnmreader_create(got_format, got_geometry, got_maxval, got_pixel, test)) == NULL) {
		printf("Fail: %s: pnmreader_create: could not allocate pnmreader\n", test->name);
	}
	if ((res = pnmreader_feed(pr, test->image, test->nbytes)) != test->result) {
		printf("Fail: %s: pnmreader_feed: expected %d, got %d\n", test->name, test->result, res);
	}
	pnmreader_destroy(pr);
}

static void
test1 (void)
{
	char image[] = \
		"P2\n"
		"2 3\n"
		"32 12 13 14 30 1 1\n";

	unsigned int pixels[] = { 12, 13, 14, 30, 1, 1 };

	run_test(&(struct test) {
		.image = image,
		.nbytes = sizeof(image) - 1,
		.name = "test1",
		.width = 2,
		.height = 3,
		.format = FORMAT_PGM_ASC,
		.maxval = 32,
		.pixels = pixels,
		.result = PNMREADER_FINISHED,
	});
}

static void
test2 (void)
{
	char image[] = \
		"P2#hello this is a comment\n"
		"    2\t\t\t3# another comment\n"
		"32 12 13\t\t14 30 1 1#and yet again a comment";

	unsigned int pixels[] = { 12, 13, 14, 30, 1, 1 };

	run_test(&(struct test) {
		.image = image,
		.nbytes = sizeof(image) - 1,
		.name = "test2",
		.width = 2,
		.height = 3,
		.format = FORMAT_PGM_ASC,
		.maxval = 32,
		.pixels = pixels,
		.result = PNMREADER_FINISHED
	});
}

static void
test3 (void)
{
	char image[] = "~~~~~~";

	run_test(&(struct test) {
		.image = image,
		.nbytes = sizeof(image) - 1,
		.name = "test3",
		.result = PNMREADER_NO_SIGNATURE
	});
}

static void
test4 (void)
{
	char image[] = "P6 1 1 70000# more than two bytes!";

	run_test(&(struct test) {
		.image = image,
		.nbytes = sizeof(image) - 1,
		.name = "test4",
		.format = FORMAT_PPM_BIN,
		.width = 1,
		.height = 1,
		.result = PNMREADER_UNSUPPORTED
	});
}

static void
test5 (void)
{
	char image[] = { 'P', '4', ' ', '1', '6', ' ', '1', ' ', 0x41, 0x12 };

	unsigned int pixels[] = {
		0, 1, 0, 0, 0, 0, 0, 1,
		0, 0, 0, 1, 0, 0, 1, 0
	};
	run_test(&(struct test) {
		.image = image,
		.nbytes = sizeof(image),
		.name = "test5",
		.width = 16,
		.height = 1,
		.format = FORMAT_PBM_BIN,
		.maxval = 1,
		.pixels = pixels,
		.result = PNMREADER_FINISHED
	});
}

static void
test6 (void)
{
	// Two-byte binary PPM:
	unsigned char image[] = {
		'P', '6', '\n',
		'2', ' ', '2', '\n',
		'6', '5', '5', '3', '5', '\n',
		0x90, 0x33, 0, 0, 0, 0, 0x78, 0x26, 0, 0, 0, 0,
		0x75, 0x98, 0, 0, 0, 0, 0x32, 0x53, 0, 0, 0, 0 };

	unsigned int pixels[] = {
		0x9033, 0x7826, 0x7598, 0x3253
	};
	run_test(&(struct test) {
		.image = (char *)image,
		.nbytes = sizeof(image),
		.name = "test6",
		.width = 2,
		.height = 2,
		.format = FORMAT_PPM_BIN,
		.maxval = 65535,
		.pixels = pixels,
		.result = PNMREADER_FINISHED
	});
}

int
main (void)
{
	test1();
	test2();
	test3();
	test4();
	test5();
	test6();
}
