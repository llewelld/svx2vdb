///////////////////////////////////////////////////////////////////
// svx2vdb
// Convert files from Simple Voxel SVX to OpenVDB format
//
// David Llewellyn-Jones
// http://www.flypig.co.uk
//
// May 2015
// MIT Licence
//
///////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////
// Includes

#include <stdlib.h>
#include <png.h>
#include <openvdb/openvdb.h>
#include <openvdb/tools/LevelSetUtil.h>
#include <zip.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>

///////////////////////////////////////////////////////////////////
// Defines

#define ERROR (0)
#define NOT_PNG (1)

///////////////////////////////////////////////////////////////////
// Structures and enumerations

typedef enum {
	ORIENTATION_INVALID = -1,
	
	ORIENTATION_X,
	ORIENTATION_Y,
	ORIENTATION_Z,
	
	ORIENTATION_NUM
} ORIENTATION;

///////////////////////////////////////////////////////////////////
// Global variables

///////////////////////////////////////////////////////////////////
// Function prototypes

void read_row_callback (png_structp ptr, png_uint_32 row, int pass);
void outputvdb ();
void createSlice (openvdb::FloatGrid & grid, png_byte ** rows, int width, int height, int z);
void createvdb (png_byte ** rows, int width, int height);
int loadPNG (png_byte *** rows, int * width, int * height, struct zip * archive, char const * filename);
openvdb::FloatGrid::Ptr initialiseVDB ();
void finaliseVDB (openvdb::FloatGrid::Ptr grid, char const * outfile);
void freePNG (png_byte ** rows, int height);
void readData (png_structp png_ptr, png_bytep data, png_size_t length);
unsigned char * readChannel (struct zip * archive, int * depth);

///////////////////////////////////////////////////////////////////
// Function definitions


int main (int argc, const char **argv) {
	png_byte ** rows;
	int width;
	int height;
	int z;
	int depth;
	openvdb::FloatGrid::Ptr grid;
	char filename[256];
	struct zip * archive;
	int error = 0;
	char * slices;
	int sliceNum;

	if (argc != 3) {
		printf ("Usage: svx2vdb [infile.svx] [outfile.vdb]\n");	
		printf ("Which: Converts Simple Voxel format files into OpenVDB files.\n");	
	}
	else {
		printf ("Reading SVX file: %s\n", argv[1]);
		archive = zip_open (argv[1], 0, & error);
		sliceNum = 0;
		printf ("Reading manifest\n");
		slices = (char *)readChannel (archive, & sliceNum);

		if ((slices) && (sliceNum > 0)) {
			grid = initialiseVDB ();

			printf ("Reading voxel slices\n");
			depth = sliceNum;
			for (z = 0; z < depth; z++) {
				sprintf (filename, slices, z);
				//printf ("%s\n", filename);
				loadPNG (& rows, & width, & height, archive, filename);
				createSlice (*grid, rows, width, height, (z - (depth / 2)));
				freePNG (rows, height);
			}

			printf ("Saving VDB file: %s\n", argv[2]);
			finaliseVDB (grid, argv[2]);
			xmlFree(slices);
		}

		zip_discard (archive);
	}
}

void readData (png_structp png_ptr, png_bytep data, png_size_t length) {
	struct zip_file * fp;
	fp = (zip_file *) png_ptr->io_ptr;

	zip_fread (fp, data, length);
}

int loadPNG (png_byte *** rows, int * width, int * height, struct zip * archive, char const * filename) {
	unsigned int number = 8;
	png_byte header[8];
	struct zip_file * fp;
	int is_png;
	unsigned int png_transforms;
	int row;
	png_byte color_type;
	png_byte bit_depth;
	png_byte colour;
	int row_count;
	png_structp png_ptr;

	fp = zip_fopen (archive, filename, 0);

	if (!fp) {
		printf ("Error reading file\n");
		return (ERROR);
	}

	zip_fread (fp, header, number);
	is_png = !png_sig_cmp (header, 0, number);
	if (!is_png) {
		printf ("Not a PNG file\n");
		return (NOT_PNG);
	}
	
	png_ptr = png_create_read_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr) {
		printf ("Setup error\n");
		return (ERROR);
	}

	png_infop info_ptr = png_create_info_struct (png_ptr);
	if (!info_ptr) {
		printf ("Info creation error\n");
		png_destroy_read_struct (&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
		return (ERROR);
	}

	png_infop end_info = png_create_info_struct (png_ptr);
	if (!end_info) {
		printf ("End info error\n");
		png_destroy_read_struct (&png_ptr, &info_ptr, (png_infopp)NULL);
		return (ERROR);
	}

	if (setjmp (png_jmpbuf (png_ptr))) {
		png_destroy_read_struct (&png_ptr, &info_ptr, &end_info);
		zip_fclose (fp);
		printf ("Read error\n");
		return (ERROR);
	}

	png_set_read_fn (png_ptr, NULL, readData);

	png_init_io (png_ptr, (png_FILE_p)fp);
	png_set_sig_bytes (png_ptr, number);
	png_set_read_status_fn (png_ptr, read_row_callback);

	png_transforms = PNG_TRANSFORM_STRIP_16 | PNG_TRANSFORM_INVERT_ALPHA | PNG_TRANSFORM_PACKING;

	png_read_info (png_ptr, info_ptr);
	*width = png_get_image_width (png_ptr, info_ptr);
	*height = png_get_image_width (png_ptr, info_ptr);


	png_byte colour_type = png_get_color_type (png_ptr, info_ptr);
	bit_depth = png_get_bit_depth (png_ptr, info_ptr);

	*rows = (png_byte **)malloc (sizeof(png_byte *) * *height);
	for (row = 0; row < *height; row++) {
		(*rows)[row] = (png_byte *)malloc (png_get_rowbytes(png_ptr, info_ptr));
	}

	png_read_image (png_ptr, *rows);

	png_read_end (png_ptr, end_info);

	zip_fclose (fp);

	png_destroy_read_struct (&png_ptr, &info_ptr, &end_info);
}

void freePNG (png_byte ** rows, int height) {
	int row;

	for (row = 0; row < height; row++) {
		free (rows[row]);
	}
	free (rows);
}

void read_row_callback (png_structp ptr, png_uint_32 row, int pass) {
	//printf ("Read row %lu\n", row);
}

void createSlice (openvdb::FloatGrid & grid, png_byte ** rows, int width, int height, int z) {
	int x;
	int y;
	openvdb::Coord ijk;
	openvdb::FloatGrid::Accessor accessor = grid.getAccessor();

	for (x = 0; x < width; x++) {
		for (y = 0; y < height; y++) {
			ijk[0] = x - (width / 2);
			ijk[1] = y - (height / 2);
			ijk[2] = z;
			if (rows[x][y] != 0) {
				accessor.setValue (ijk, 1.0);
			}
		}
	}
}

openvdb::FloatGrid::Ptr initialiseVDB () {
	openvdb::initialize ();
	openvdb::FloatGrid::Ptr grid = openvdb::FloatGrid::create (2.0);

	return grid;
}

void finaliseVDB (openvdb::FloatGrid::Ptr grid, char const * outfile) {
	grid->setTransform (openvdb::math::Transform::createLinearTransform (0.5));
	grid->setGridClass (openvdb::GRID_FOG_VOLUME);
	grid->setName ("LevelSetSVX");
	openvdb::io::File file (outfile);
	openvdb::GridPtrVec grids;
	grids.push_back (grid);

	//openvdb::tools::sdfToFogVolume<openvdb::FloatGrid>(grid.operator*());

	file.write (grids);
	file.close ();
}

unsigned char * readChannel (struct zip * archive, int * depth) {
	xmlDocPtr doc;
	char * data;
	struct zip_file * fp;
	struct zip_stat sb;
	int result;
	xmlXPathObjectPtr xpathresult;
	xmlXPathContextPtr context;
	unsigned char * slices;
	unsigned char * orientation;
	ORIENTATION orient;
	unsigned char * gridSize;
	unsigned const char * expression;
	int entryNum;
	unsigned char * key;
	unsigned char * value;

	slices = NULL;
	if (depth != NULL) {
		*depth = 0;
	}
	result = zip_stat (archive, "manifest.xml", ZIP_FL_NOCASE, & sb);

	if ((sb.valid & ZIP_STAT_SIZE) != 0) {
		data = (char *)malloc (sb.size);
		if (data != NULL) {
			fp = zip_fopen (archive, "manifest.xml", 0);

			zip_fread (fp, data, sb.size);

			zip_fclose (fp);

			doc = xmlReadMemory (data, sb.size, "noname.xml", NULL, 0);
			if (doc == NULL) {
				printf ("Error reading XML\n");
			}
			else {
				context = xmlXPathNewContext (doc);
				xpathresult = xmlXPathEvalExpression ((unsigned char *)"/grid/channels/channel[@type = \"DENSITY\"]/@slices", context);

				if (xpathresult != NULL) {
					if (xpathresult->nodesetval->nodeNr > 0) {
						slices = xmlNodeListGetString (doc, xpathresult->nodesetval->nodeTab[0]->xmlChildrenNode, 1);
						printf ("\tSlice name format: %s\n", slices);
					}
					free (xpathresult);
				}


				xpathresult = xmlXPathEvalExpression ((unsigned char *)"/grid/@slicesOrientation", context);

				if (xpathresult != NULL) {
					if (xpathresult->nodesetval->nodeNr > 0) {
						orientation = xmlNodeListGetString (doc, xpathresult->nodesetval->nodeTab[0]->xmlChildrenNode, 1);
						
						if (strlen((char *)orientation) > 0) {
							printf ("\tOrientation: %s\n", orientation);
							switch (orientation[0]) {
								case 'X':
								case 'x':
									orient = ORIENTATION_X;
									break;
								case 'Y':
								case 'y':
									orient = ORIENTATION_Y;
									break;
								case 'Z':
								case 'z':
									orient = ORIENTATION_Z;
									break;
								default:
									orient = ORIENTATION_INVALID;
									break;
							}
						}
						
						xmlFree (orientation);
					}
					free (xpathresult);
				}

				switch (orient) {
					case ORIENTATION_X:
						expression = (unsigned char *)"/grid/@gridSizeX";
						break;
					case ORIENTATION_Y:
						expression = (unsigned char *)"/grid/@gridSizeY";
						break;
					case ORIENTATION_Z:
						expression = (unsigned char *)"/grid/@gridSizeZ";
						break;
					default:
						expression = (unsigned char *)"";
				}

				xpathresult = xmlXPathEvalExpression (expression, context);

				if (xpathresult != NULL) {
					if (xpathresult->nodesetval->nodeNr > 0) {
						gridSize = xmlNodeListGetString (doc, xpathresult->nodesetval->nodeTab[0]->xmlChildrenNode, 1);
						if (depth != NULL) {
							sscanf((char *)gridSize, "%d", depth);
							printf ("\tDepth: %d\n", *depth);
						}
						
						xmlFree (gridSize);
					}
					free (xpathresult);
				}

				xpathresult = xmlXPathEvalExpression ((unsigned char *)"/grid/metadata/entry", context);

				if (xpathresult != NULL) {
					for (entryNum = 0; entryNum < xpathresult->nodesetval->nodeNr; entryNum++) {
						xmlNodePtr cur = xpathresult->nodesetval->nodeTab[entryNum];
						key = xmlGetProp (cur, (unsigned char *)"key");
						if (key != NULL) {
							value = xmlGetProp (cur, (unsigned char *)"value");
							if (value != NULL) {
								printf ("\t%s: %s\n", key, value);
								xmlFree (value);
							}
							xmlFree (key);
						}
					}
					free (xpathresult);
				}
			
				if (context) {
					free (context);
				}
			
				xmlFreeDoc (doc);
			}
			xmlCleanupParser ();
		}
	}
	
	return slices;
}


