ftype=`freetype-config --cflags --libs`
libjpeg_flags=-I/usr/local/include -l jpeg
CFLAGS=${ftype} ${libjpeg_flags}

default: glyph2jpg
