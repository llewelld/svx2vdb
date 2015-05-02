# svx2vdb

Convert Simple Voxel SVX files to OpenVDB files.

The code needs to be cleaned up and a lot more testing done, but it should work for most SVX files.

For info about the SVX format, please see this [post about it](http://abfab3d.com/svx-format/).

For info about OpenVDB, please check the [OpenVDB website](http://www.openvdb.org/).

One nice benefit of converting files in this way is that it allows voxel clouds to be viewed using the [vdb_view](http://www.openvdb.org/download/) utility or [Houdini](http://www.sidefx.com/) tools. Currently there aren't any tools to view SVX files directly, as far as I'm aware.

An example SVX file is provided in the **examples** folder. Shapeways also provide a [sphere](http://shapeways.com/rrstatic/files/sphere.svx) example, or you can generate your own files using [Functy](http://functy.sourceforge.net/).

## Dependencies

The current dependencies for building svx2vdb are:

1. [OpenVDB](http://www.openvdb.org/) for OpenVDB voxel cloud creation.
1. [OpenEXR](http://www.openexr.com/) as an OpenVDB dependency.
1. [libtbb2](https://www.threadingbuildingblocks.org/) for OpenVDB threading.
1. [libpng](http://libpng.org/pub/png/libpng.html) for reading SVX slices.
1. [libzip](http://www.nih.at/libzip/) for extracting SVX file contents.
1. [libxml-2.0](http://xmlsoft.org/index.html) for interpreting  the SVX manifest.

If you're using Linux, most if not all of these should be in the repositories (at least this is the case for Ubuntu 14.10 which I'm usin here).

## Build Me

The build bash script should work on most Linux distributions. Simple type
```
build svx2vdb
```
To create the `svx2vdb` binary. This does nothing clever, so won't pull in or warn about missing dependencies. Eventually I'll move this over to use autobuild to make things smoother.
