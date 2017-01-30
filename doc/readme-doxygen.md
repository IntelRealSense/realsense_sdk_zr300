# Documentation
These files are used to configure the dynamic Documentation generation for this project.

## Building Documentation

The following steps will generate the Doxygen based Documentation for this project:
```bash
$ mkdir build
$ cd build
$ cmake -DBUILD_DOCUMENTATION=on ..
$ make doc
```

## Attributions
-This documentation uses Doxygen version 1.8.11 (a change made in 1.8.12 is not compatible with this project).  For download link, see https://sourceforge.net/projects/doxygen/files/rel-1.8.11/
-This documentation uses the style sheet posted by 'Velron' at https://github.com/Velron/doxygen-bootstrapped, and is utilized under it's provided Apache 2.0 license.

