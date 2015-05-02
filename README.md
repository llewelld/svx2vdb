# svx2vdb

Convert Simple Voxel SVX files to OpenVDB files.

The code needs to be cleaned up and a lot more testing done, but it should work for most SVX files.

For info about the SVX format, please see this [post about it](http://abfab3d.com/svx-format/).

For info about OpenVDB, please check the [OpenVDB website](http://www.openvdb.org/).

One nice benefit of converting files in this way is that it allows voxel clouds to be viewed using the [vdb_view](http://www.openvdb.org/download/) utility or [Houdini](http://www.sidefx.com/) tools. Currently there aren't any tools to view SVX files directly, as far as I'm aware.

An example SVX file is provided in the **examples** folder. Shapeways also provide a [sphere](http://shapeways.com/rrstatic/files/sphere.svx) example, or you can generate your own files using [Functy](http://functy.sourceforge.net/).
