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
