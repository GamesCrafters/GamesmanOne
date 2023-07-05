# GamesmanExperiment

## Steps to Compile New Source Files

1. Run `autoscan` in the base directory. This will generate a new file called `configure.scan`.
2. Make the following changes to `configure.scan`:
    - Modify the line that starts with `AC_INIT` with the proper package name, version, and bug report address.
    - Add the following as a new line after the line that starts with `AC_INIT`: `AM_INIT_AUTOMAKE([-Wall -Werror subdir-objects foreign])`. This initializes Automake with options that tell it to treat warnings as errors, enable compiling with subdirectory objects, and not complain about missing files required by the GNU standard.
    - Modify the line that starts with `AC_CONFIG_SRCDIR` so that it points to an arbitrary file in `/src/`.
3. Rename `configure.scan` to `configure.ac`.
4. Modify `Makefile.am` so that it includes the new sources that you added.
5. Add any additional compiler flags and linker directives as necessary.
6. `autoreconf` in the base directory.
7. `./configure`.
8. `make`.
