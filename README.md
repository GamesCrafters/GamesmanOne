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

GamesmanOne is built with [CMake](https://cmake.org/cmake-tutorial/), which can take the following options:

Variable                     | Type   | Description
-----------------------------|--------|--------------
CMAKE_BUILD_TYPE             | String | Project build type.
DISABLE_OPENMP               | Bool   | Defaults to `ON`. Set this to `OFF` to disable OpenMP.
USE_MPI                      | Bool   | Defaults to `OFF`. Set this to `ON` to enable MPI.

Each one of the variables can be set at configure time by passing a definition of the variable to the `cmake` command.
As an example, running `cmake -DCMAKE_BUILD_TYPE=Release -DUSE_MPI=ON build` under the root project directory builds
the project in `Release` mode with both OpenMP and MPI enabled.

In particular, the build type of the project should ALWAYS be set by providing a String value to `CMAKE_BUILD_TYPE`.
The default build type used by the installer script is `Release`, which enables compiler optimizations and disables `assert` macros.
To reconfigure the project in `Debug` mode with debug symbols attached and compiler optimizations turned off, run
`cmake -DCMAKE_BUILD_TYPE=Release build` under the root project directory. 
[Documentation is available here](https://cmake.org/cmake/help/latest/variable/CMAKE_BUILD_TYPE.html).

## Steps to Add New Source Files

For each new filed added, modify or create a new `CMakeLists.txt` under the same directory of the file to include the file for compilation.
You may need to modify `gamesman_install.sh` if, for example, you are linking new libraries as submodules.

## Building GamesmanOne on the Savio Cluster

1. Clone GamesmanOne
    `git clone https://github.com/GamesCrafters/GamesmanOne.git`
2. Run `module load gcc/11.3.0 ucx/1.14.0 openmpi/5.0.0-ucx` if using the `savio4_htc` partition. For all other partitions, run `module load gcc openmpi`. Note that you will need to re-run one of the above `module load` commands and recompile GamesmanOne if you decided to switch to or from the `savio4_htc` partition as the modules loaded at runtime depends on the partition selected.
3. Clean up previous build by running `cmake --build build --target clean` under the root project directory, or `make clean` under the build directory.
4. Reconfigure `cmake` by running `cmake -DCMAKE_BUILD_TYPE=Release -DUSE_MPI=ON build` under the root project directory.
5. Rebuild the project with `cmake --build build` under the root project directory.
