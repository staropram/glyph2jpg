#include <stdio.h>
#include <errno.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <jpeglib.h>

// globals
FT_Library ftlib;
FT_Face ftface;

/**
 * Function to render glyph to jpg.
 *
 * @param fname The filename to render to.
 * @param bitmap The FT_Bitmap generated from the font and character.
 * @param target_width The width of the generated image before paddding.
 * @param target_height The width of the generated image before padding.
 * @param xpad Horizontal padding on both sides of the image.
 * @param ypad Vertical padding on both sides of the image.
 * @param depth Image depth, 1 for grayscale , 3 for RGB.
 *
 * @return 0 on success, 1 otherwise.
 */
int render_glyph_to_jpg(
	char *fname,
	FT_Bitmap *bitmap, 
	int target_width,
	int target_height,
	int xpad,
	int ypad,
	int depth
) {

	int width  = target_width+2*xpad;
	int height = target_height+2*ypad;

	if(bitmap->width > target_width) {
		fprintf(stderr,"Error, target width %d is smaller than font width %d!\n",target_width,bitmap->width);
		return 1;
	}

	if(bitmap->rows > target_height) {
		fprintf(stderr,"Error, target height %d is smaller than font height %d!\n",target_height,bitmap->rows);
		return 1;
	}

	// open file for binary writing
	FILE *f = fopen(fname,"wb");

	if(target_width > bitmap->width) {
		xpad += ((target_width - bitmap->width)/2);
	}

	if(target_height  > bitmap->rows) {
		ypad += ((target_height - bitmap->rows));
	}

	unsigned char *buffer = malloc(width*height*depth);
	int p = 0, bitmap_p = 0;
	for(int y=0; y<height; y++) {
		for(int x=0; x<width; x++) {

			// make padding white
			if( ( y < ypad ) ||
				 ( y >= (ypad+bitmap->rows) ) ||
				 ( x < xpad ) || 
				 ( x >= (xpad+bitmap->width) )
			  ) {
				for(int d=0; d<depth; d++) {
					buffer[p++] = 0xFF;
				}
				continue;
			} 
			
			for(int d=0; d<depth; d++) {
				buffer[p++] = 0xFF-bitmap->buffer[bitmap_p];
			}
			bitmap_p++;
		}
	}

	// jpeglib
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr       jerr;
	 
	// setup jpeg error handling 
	cinfo.err = jpeg_std_error(&jerr);
	 
	// initialize the compression object
	jpeg_create_compress(&cinfo);
	// set the output to the file
	jpeg_stdio_dest(&cinfo, f);

	// fill in the compression structure
	// width, height in pixels and depth
	cinfo.image_width      = width;
	cinfo.image_height     = height;
	cinfo.input_components = depth;
	if(depth==1) {
		cinfo.in_color_space = JCS_GRAYSCALE;
	} else if(depth==3) {
		cinfo.in_color_space = JCS_RGB;
	} else {
		fprintf(stderr,"Invalid color space, valid depths are 1 and 3\n");
		fclose(f);
		jpeg_destroy_compress(&cinfo);
		free(buffer);
		return 1;
	}

	// set the compression details
	jpeg_set_defaults(&cinfo);

	// set quality
	jpeg_set_quality(&cinfo, 100, 1);
	jpeg_start_compress(&cinfo, 1);

	// iterate over each row of image buffer
	JSAMPROW row_pointer;
	 
	cinfo.next_scanline = 0;
	int row_stride = cinfo.input_components*cinfo.image_width;
	while (cinfo.next_scanline < cinfo.image_height) {
		row_pointer = (JSAMPROW) &buffer[cinfo.next_scanline*row_stride];
		jpeg_write_scanlines(&cinfo, &row_pointer, 1);
	}

	jpeg_finish_compress(&cinfo);

	fclose(f);

	jpeg_destroy_compress(&cinfo);

	free(buffer);

	return 0;
}


/**
 * Routine to grab an integer from a character string.
 *
 * @param input The character string to process.
 * @param base The base that the number is encoded in.
 * @param output Pointer to int to store the conversion in.
 *
 * @return 1 on error, 0 otherwise
 */
int get_integer(char* input, int base, int *output, char *variable_name) {
	char *endptr = NULL;
	errno = 0;
	*output = (int) strtol(input,&endptr,base);

	if(endptr==input) {
		fprintf(stderr,"Endptr equals input string, no characters found\n");
		if(errno) {
			fprintf(stderr,"errno is not zero: %d\n",errno);
			if(errno==EINVAL) {
				fprintf(stderr,"errno is EINVAL\n");
			} else if(errno==ERANGE) {
				fprintf(stderr,"errno is ERANGE\n");
			}
		}
		fprintf(stderr,"Error reading %s. Expected number, got \"%s\"\n",
			variable_name,
			input
		);
		return 1;
	}
	return 0;
}

int main(int argc, char **argv) {
	// locals
	char *fontpath = NULL, *outjpg = NULL, *glyph = NULL;
	int argp = 1, width = 0, height = 0, xpad = 0, ypad = 0,
		 depth = 0, error = 0, glyphi = 0, glyph_size = 0;

	// process cmdline args
	if(argc!=10) {
		printf("USAGE:\n\tglyph2jpg path_to_font width height xpad ypad depth glyph glyph_size out.jpg\n");
		printf("\n\tdepth is either 1 for grayscale or 3 for RGB\n");
		return 0;
	}

	fontpath = argv[argp++];
	if(get_integer(argv[argp++],10,&width, "width" )) { return 1; }
	if(get_integer(argv[argp++],10,&height,"height")) { return 1; }
	if(get_integer(argv[argp++],10,&xpad,  "xpad"  )) { return 1; }
	if(get_integer(argv[argp++],10,&ypad,  "ypad"  )) { return 1; }
	if(get_integer(argv[argp++],10,&depth, "depth" )) { return 1; }
	if(get_integer(argv[argp++],10,&glyphi,"glyphi")) { return 1; }
	if(get_integer(argv[argp++],10,&glyph_size,"glyph_size")) { return 1; }
	outjpg = argv[argp++];

	// init freetype
	error = FT_Init_FreeType(&ftlib);
	if(error) {
		fprintf(stderr,"An error occurred initializing freetype!\n");
		fprintf(stderr,"Error number: %d\n",error);
		return 1;
	}

	// load the face at index 0 from the specified font
	error = FT_New_Face(
		ftlib,
		fontpath,
		0, // font index
		&ftface
	);

	if(error) {
		if(error== FT_Err_Unknown_File_Format) {
			fprintf(stderr,"Unknown font file format!\n");
		} else {
			fprintf(stderr,"Error loading font from path!\n");
		}
		fprintf(stderr,"Error number: %d\n",error);
		return 1;
	}

	// set the font size in pixels
	error = FT_Set_Pixel_Sizes(
		ftface,
		width,
		height	
	);

	error = FT_Set_Char_Size(
		ftface,
		glyph_size*64,
		0,
		300,
		300
	);

	if(error) {
		fprintf(stderr,"Error setting pixel sizes!\n");
		fprintf(stderr,"Error number: %d\n",error);
		return 1;
	}

	// get the glyph index for the character
	int glyph_index = FT_Get_Char_Index(ftface, glyphi);

	// load the glyph into the glyph "slot" of the face object
	error = FT_Load_Glyph(
		ftface,      // handle to face object
		glyph_index, // glyph index
		FT_LOAD_DEFAULT // load flags, see below
	);

	if(error) {
		fprintf(stderr,"Error loading glyph!\n");
		return 1;
	}

	// render the glyph
	error = FT_Render_Glyph(
		ftface->glyph,        // glyph slot
      FT_RENDER_MODE_NORMAL // render mode
	);

	// render the glyph to the output file
	FT_GlyphSlot slot = ftface->glyph;
	error = render_glyph_to_jpg(
		outjpg,
		&slot->bitmap,
		width,
		height,
		xpad,
		ypad,
		depth	
	);
	if(error) {
		fprintf(stderr,"Error rendering glyph!\n");
		fprintf(stderr,"Error number: %d\n",error);
		return 1;
	}

	return 0;

}
