#!/bin/sh
font=Chunkfive.otf
width=990
height=990
xpad=5
ypad=5
depth=1
output_dir=output
output_base=out
glyphcode=65
glyph_size=229

mkdir -f output

while [ $glyphcode -le 122 ]; do
	echo $glyphcode

	./glyph2jpg \
		$font \
		$width \
		$height \
		$xpad \
		$ypad \
		$depth \
		$glyphcode \
		$glyph_size \
		${output_dir}/${output_base}_${glyphcode}.jpg

	glyphcode=$(expr $glyphcode + 1)

done
