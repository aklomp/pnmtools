#!/bin/bash

cmp ()
{
	compare -metric PSNR "$1" "$2" /dev/null
}

# Some of the pnmtools do not generate repeatable output (!),
# so use actual files for testing:
GRADIENT_PBM_ASC=/dev/shm/gradient.asc.pbm
GRADIENT_PGM_ASC=/dev/shm/gradient.asc.pgm
GRADIENT_PPM_ASC=/dev/shm/gradient.asc.ppm
GRADIENT_PBM_BIN=/dev/shm/gradient.bin.pbm
GRADIENT_PGM_BIN=/dev/shm/gradient.bin.pgm
GRADIENT_PPM_BIN=/dev/shm/gradient.bin.ppm

pamgradient \#a0b0c0 \#203040 \#8070d0 \#10d070 80 80 | pamtopnm > $GRADIENT_PPM_BIN
ppmtopgm < $GRADIENT_PPM_BIN > $GRADIENT_PGM_BIN
pgmtopbm < $GRADIENT_PGM_BIN > $GRADIENT_PBM_BIN
pnmtoplainpnm < $GRADIENT_PPM_BIN > $GRADIENT_PPM_ASC
pnmtoplainpnm < $GRADIENT_PGM_BIN > $GRADIENT_PGM_ASC
pnmtoplainpnm < $GRADIENT_PBM_BIN > $GRADIENT_PBM_ASC

for BUFSIZE in `seq 1 10`
do
	cmp \
	  <( pgmmake 0.5 7 9 ) \
	  <( pgmmake 0.5 7 9 | ./simplecopy $BUFSIZE )

	cmp \
	  <( pgmmake 0.5 7 9 ) \
	  <( pgmmake 0.5 7 9 | pnmtoplainpnm | ./simplecopy $BUFSIZE )

	cmp $GRADIENT_PPM_BIN <( ./simplecopy $BUFSIZE < $GRADIENT_PPM_BIN )
	cmp $GRADIENT_PGM_BIN <( ./simplecopy $BUFSIZE < $GRADIENT_PGM_BIN )
	cmp $GRADIENT_PBM_BIN <( ./simplecopy $BUFSIZE < $GRADIENT_PBM_BIN )
	cmp $GRADIENT_PPM_ASC <( ./simplecopy $BUFSIZE < $GRADIENT_PPM_ASC )
	cmp $GRADIENT_PGM_ASC <( ./simplecopy $BUFSIZE < $GRADIENT_PGM_ASC )
	cmp $GRADIENT_PBM_ASC <( ./simplecopy $BUFSIZE < $GRADIENT_PBM_ASC )

	for wd in `seq 1 10`
	do
		for ht in `seq 1 4`
		do
			A=/dev/shm/pbm.pbm
			pgmmake 0.5 $wd $ht | pgmtopbm > $A

			cmp $A <( ./simplecopy $BUFSIZE < $A )

			cmp \
			  <( pnmtoplainpnm < $A ) \
			  <( pnmtoplainpnm < $A | ./simplecopy $BUFSIZE )

			rm $A
		done
	done
done

rm \
  $GRADIENT_PBM_ASC \
  $GRADIENT_PGM_ASC \
  $GRADIENT_PPM_ASC \
  $GRADIENT_PBM_BIN \
  $GRADIENT_PGM_BIN \
  $GRADIENT_PPM_BIN \
