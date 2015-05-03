# svx2vdb

Convert Simple Voxel SVX files to OpenVDB files.

The code needs to be cleaned up and a lot more testing done, but it should work for most SVX files.

For info about the SVX format, please see this [post about it](http://abfab3d.com/svx-format/).

For info about OpenVDB, please check the [OpenVDB website](http://www.openvdb.org/).

One nice benefit of converting files in this way is that it allows voxel clouds to be viewed using the [vdb_view](http://www.openvdb.org/download/) utility or [Houdini](http://www.sidefx.com/) tools. Currently there aren't any tools to view SVX files directly, as far as I'm aware.

## Usage

This is a simple command-line tool that takes two parameters.
```
svx2vdb infile.svx outfile.vdb
```
The first parameter `infile.svx` should be an existing [SVX](http://abfab3d.com/svx-format/) format file to read in. This will be converted to the [OpenVDB](http://www.openvdb.org/) format file written to `outfile.vdb`.

If you have the [OpenVDB tools](http://www.openvdb.org/download/) installed, you can then view the resulting output file as follows.
```
vdb_view outfile.vdb
```
An example SVX file is provided in the **examples** folder. Shapeways also provide a [sphere](http://shapeways.com/rrstatic/files/sphere.svx) example, or you can generate your own files using [Functy](http://functy.sourceforge.net/).

## Build Me

You can use the autotools configure script to build svx2vdb on Linux systems. Set the current working directory to the svx2vdb folder, then enter the following.
```
./configure
make
```
If the configure step generates errors, it's likely because you need to install autotools, or one of the code dependencies listed below.

If there are no errors, this will build the svx2vdb binary in the same folder. You can then test it by converting one of the example files and viewing the result.
```
./svx2vdb examples/urchin.svx out.vdb
vdb_view out.vdb
```
You can then press the `3` key to show the voxels.

Use `make install` as root to install it into the path.

Use `make check` to run a basic execution test on the code.

## Dependencies

The current dependencies for building svx2vdb are:

1. [OpenVDB](http://www.openvdb.org/) for OpenVDB voxel cloud creation.
1. [OpenEXR](http://www.openexr.com/) as an OpenVDB dependency.
1. [libtbb2](https://www.threadingbuildingblocks.org/) for OpenVDB threading.
1. [libpng](http://libpng.org/pub/png/libpng.html) for reading SVX slices.
1. [libzip](http://www.nih.at/libzip/) for extracting SVX file contents.
1. [libxml-2.0](http://xmlsoft.org/index.html) for interpreting  the SVX manifest.

If you're using Linux, most if not all of these should be in the repositories (at least this is the case for Ubuntu 14.10 which I'm using here):

```
sudo apt-get install autoconf
sudo apt-get install libopenvdb-dev
sudo apt-get install libopenvdb-tools
sudo apt-get install libopenexr-dev
sudo apt-get install libtbb-dev
sudo apt-get install libpng12-dev
sudo apt-get install libzip-dev
sudo apt-get install libxml2-dev
```

## License

Read COPYING for information on the license. svx2vdb is released under the MIT License.

## Contact and Links

More information can be found at: http://www.flypig.co.uk?to=linux
The source code is avilable from GitHub: https://github.com/llewelld/svx2vdb

I can be contacted via one of the following.

 * My website: http://www.flypig.co.uk
 * Email: at david@flypig.co.uk

