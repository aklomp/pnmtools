#ifndef PNMWRITER_H
#define PNMWRITER_H

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

struct pnmwriter * pnmwriter_create (FILE *file);

// Destroy the pnmreader struct:
void pnmwriter_destroy (struct pnmwriter *const);

bool pnmwriter_format (struct pnmwriter *const, enum pnm_format format);

bool pnmwriter_width (struct pnmwriter *const, unsigned int width);

bool pnmwriter_height (struct pnmwriter *const, unsigned int height);

bool pnmwriter_maxval (struct pnmwriter *const, unsigned int maxval);

bool pnmwriter_pixel (struct pnmwriter *const, unsigned int r, unsigned int g, unsigned int b);

#endif
