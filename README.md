# GamesmanOne

## Installing GamesmanOne

1. Clone GamesmanOne
    `git clone https://github.com/GamesCrafters/GamesmanOne.git`
2. Run the installer script in the base directory
    ```
    cd GamesmanOne
    bash ./gamesman_install.sh
    ```
   GamesmanOne will be automatically configured and installed. Run `bin/gamesman` to launch the system.

## Configure Options

1. The `configure` script can be run with different options. Here is a list of commonly used commands:
    - `./configure 'CFLAGS=-Wall -Wextra -g -O0' --disable-openmp` for debugging on a single thread.
    - `./configure 'CFLAGS=-Wall -Wextra -g -O0'` for debugging with OpenMP multithreading enabled.
    - `./configure 'CFLAGS=-Wall -Wextra -g -O3 -DNDEBUG' --disable-openmp` for solving on a single thread.
    - `./configure 'CFLAGS=-Wall -Wextra -g -O3 -DNDEBUG'` for solving on multiple threads.
2. After running the `configure` script, run `make`.

## Steps to Add New Source Files

1. Run `autoscan` in the base directory. This will generate a new file called `configure.scan`. This step generates no output if the current `configure.ac` already covers all the necessary checks. If so, skip to step 3. Otherwise, proceed to step 2.
2. Inspect `configure.scan` and compare with `configure.ac`. Add missing library and function checks to `configure.ac` and run `autoscan` again. The command should give no output if all checks are correctly implemented.
3. Include new sources in `Makefile.am`.
4. Add any additional compiler flags and linker directives as necessary.
5. Modify `gamesman_install.sh` if necessary (e.g., linking new libraries as submodules.)

## Building GamesmanOne on the Savio Cluster

1. Clone GamesmanOne
    `git clone https://github.com/GamesCrafters/GamesmanOne.git`
2. Run `module load gcc/11.3.0 ucx/1.14.0 openmpi/5.0.0-ucx` if using the `savio4_htc` partition. For all other partitions, run `module load gcc openmpi`. Note that you will need to re-run one of the above `module load` commands and recompile GamesmanOne if you decided to switch to or from the `savio4_htc` partition as the modules loaded at runtime depends on the partition selected.
3. Run the installer script in the base directory
    ```
    cd GamesmanOne
    bash ./gamesman_install.sh
    ```
   GamesmanOne will be automatically configured and compiled but without support for MPI. Run `bin/gamesman` to launch the system and test if it works without support for MPI. Quit the program when you are done.
4. Run `make clean` to remove the current installation.
5. Run `./configure 'CFLAGS=-Wall -Wextra -g -O3 -DNDEBUG' 'CC=mpicc'` to reconfigure the system to use `mpicc` as the compiler. Make sure that `gcc` and `openmpi` modules are loaded properly in step 2.
6. Run `make` to recompile the program.
7. Run `bin/gamesman` again and check if MPI has been enabled.
