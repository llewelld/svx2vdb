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

void convertSVXtoVDB (char const * inputSVX, char const * outputVDB);
void readRow (png_structp ptr, png_uint_32 row, int pass);
void createSlice (openvdb::FloatGrid & grid, png_byte ** rows, int width, int height, int z);
int loadPNG (png_byte *** rows, int * width, int * height, struct zip * archive, char const * filename);
openvdb::FloatGrid::Ptr initialiseVDB ();
void finaliseVDB (openvdb::FloatGrid::Ptr grid, char const * outfile);
void freePNG (png_byte ** rows, int height);
void readData (png_structp png_ptr, png_bytep data, png_size_t length);
unsigned char * readManifest (struct zip * archive, int * depth);

///////////////////////////////////////////////////////////////////
// Function definitions


// The show starts here
int main (int argc, const char **argv) {
	if (argc != 3) {
		printf ("Usage: svx2vdb [infile.svx] [outfile.vdb]\n");	
		printf ("Which: Converts Simple Voxel format files into OpenVDB files.\n");	
	}
	else {
		convertSVXtoVDB (argv[1], argv[2]);
	}
}

// Load the PNG slices from inputSVX and write out a VDB file to outputVDB
void convertSVXtoVDB (char const * inputSVX, char const * outputVDB) {
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

	printf ("Reading SVX file: %s\n", inputSVX);
	archive = zip_open (inputSVX, 0, & error);
	sliceNum = 0;
	printf ("Reading manifest\n");
	slices = (char *)readManifest (archive, & sliceNum);

	if ((slices) && (sliceNum > 0)) {
		grid = initialiseVDB ();

		printf ("Reading voxel slices\n");
		depth = sliceNum;
		for (z = 0; z < depth; z++) {
			sprintf (filename, slices, z);
			loadPNG (& rows, & width, & height, archive, filename);
			createSlice (*grid, rows, width, height, (z - (depth / 2)));
			freePNG (rows, height);
			printf ("\tRead slice %d of %d\r", (z + 1), depth);
			fflush(stdout);
		}

		printf ("\nSaving VDB file: %s\n", outputVDB);
		finaliseVDB (grid, outputVDB);
		xmlFree(slices);
	}

	zip_discard (archive);
}

// Used to replace the standard file read functions for use with
// libpng. Reads the PNG direct from the zip archive
void readData (png_structp png_ptr, png_bytep data, png_size_t length) {
	struct zip_file * fp;
	fp = (zip_file *) png_ptr->io_ptr;

	zip_fread (fp, data, length);
}

// Load the PNG called filename from an archive. Returns the PNG data in rows
// along with the width and height. Each row in the rows array must be freed
// individually once the data is no longer needed. The freePNG function can
// be used for this.
int loadPNG (png_byte *** rows, int * width, int * height, struct zip * archive, char const * filename) {
	unsigned int number = 8;
	png_byte header[8];
	struct zip_file * fp;
	int is_png;
	int row;
	png_structp png_ptr;
	png_infop end_info;

	png_ptr = NULL;
	end_info = NULL;

	fp = zip_fopen (archive, filename, 0);

	if (!fp) {
		printf ("Error reading file\n");
	}
	else {
		zip_fread (fp, header, number);
		is_png = !png_sig_cmp (header, 0, number);

		if (!is_png) {
			printf ("Not a PNG file\n");
		}
		else {
			png_ptr = png_create_read_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

			if (!png_ptr) {
				printf ("Setup error\n");
			}
			else {
				png_infop info_ptr = png_create_info_struct (png_ptr);

				if (!info_ptr) {
					printf ("Info creation error\n");
				}
				else {
					end_info = png_create_info_struct (png_ptr);

					if (!end_info) {
						printf ("End info error\n");
					}
					else {

						if (setjmp (png_jmpbuf (png_ptr))) {
							png_destroy_read_struct (&png_ptr, &info_ptr, &end_info);
							zip_fclose (fp);
							printf ("Read error\n");
							return 0;
						}

						png_set_read_fn (png_ptr, NULL, readData);

						png_init_io (png_ptr, (png_FILE_p)fp);
						png_set_sig_bytes (png_ptr, number);
						png_set_read_status_fn (png_ptr, readRow);

						//png_transforms = PNG_TRANSFORM_STRIP_16 | PNG_TRANSFORM_INVERT_ALPHA | PNG_TRANSFORM_PACKING;

						png_read_info (png_ptr, info_ptr);
						*width = png_get_image_width (png_ptr, info_ptr);
						*height = png_get_image_width (png_ptr, info_ptr);


						//png_byte colour_type = png_get_color_type (png_ptr, info_ptr);
						//bit_depth = png_get_bit_depth (png_ptr, info_ptr);

						*rows = (png_byte **)malloc (sizeof(png_byte *) * *height);
						for (row = 0; row < *height; row++) {
							(*rows)[row] = (png_byte *)malloc (png_get_rowbytes(png_ptr, info_ptr));
						}

						png_read_image (png_ptr, *rows);

						png_read_end (png_ptr, end_info);
					}
				}

				png_destroy_read_struct (&png_ptr, &info_ptr, &end_info);
			}
		}
		zip_fclose (fp);
	}
	
	return 1;
}

// Free the PNG row data generated by loadPNG
void freePNG (png_byte ** rows, int height) {
	int row;

	for (row = 0; row < height; row++) {
		free (rows[row]);
	}
	free (rows);
}

// Callback called after each row of each PNG is loaded in.
// This could be used for a progress bar, but currently
// does nothing.
void readRow (png_structp ptr, png_uint_32 row, int pass) {
	// Do nothing
	//printf ("Read row %lu\n", row);
}

// Record the row data loaded in from a PNG in the OpenVDB grid
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

// Initialise the OpenVDB grid
openvdb::FloatGrid::Ptr initialiseVDB () {
	openvdb::initialize ();
	openvdb::FloatGrid::Ptr grid = openvdb::FloatGrid::create (2.0);

	return grid;
}

// Finalise and output the OpenVDB grid to file
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

// Read in relevant data from thee SVX manifest. This includes
// dimensions, orientation and the filename format used for the
// individual slices. Metadata is also output for interest.
unsigned char * readManifest (struct zip * archive, int * depth) {
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
	
	// Check the size of the manifest file so we can read it into memory
	result = zip_stat (archive, "manifest.xml", ZIP_FL_NOCASE, & sb);

	if ((result == 0) && ((sb.valid & ZIP_STAT_SIZE) != 0)) {
		// Allocate enough memory to hold the manifest file
		data = (char *)malloc (sb.size);
		if (data != NULL) {
			// Load the data into memory
			fp = zip_fopen (archive, "manifest.xml", 0);
			zip_fread (fp, data, sb.size);
			zip_fclose (fp);

			// Manage the data through an XML DOM interface
			doc = xmlReadMemory (data, sb.size, "noname.xml", NULL, 0);
			if (doc == NULL) {
				printf ("Error reading XML\n");
			}
			else {
				// Read in the filename format of the PNG slices
				context = xmlXPathNewContext (doc);
				xpathresult = xmlXPathEvalExpression ((unsigned char *)"/grid/channels/channel[@type = \"DENSITY\"]/@slices", context);

				if (xpathresult != NULL) {
					if (xpathresult->nodesetval->nodeNr > 0) {
						slices = xmlNodeListGetString (doc, xpathresult->nodesetval->nodeTab[0]->xmlChildrenNode, 1);
						printf ("\tSlice name format: %s\n", slices);
					}
					free (xpathresult);
				}

				// Read in the orientation
				xpathresult = xmlXPathEvalExpression ((unsigned char *)"/grid/@slicesOrientation", context);
				orient = ORIENTATION_INVALID;

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

				// Read in the number of slices based on the orientation
				// This is equivalent to the number of PNG slices to read in
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
						printf ("\tNo orientation stated. Assuming Y.\n");
						expression = (unsigned char *)"/grid/@gridSizeY";
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

				// Read in the metadata and output it to stdout
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
			free (data);
		}
	}
	
	return slices;
}


