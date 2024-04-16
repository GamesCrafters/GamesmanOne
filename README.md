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

TODO:
`cmake -DCMAKE_BUILD_TYPE=Release -DUSE_MPI=ON ..`

## Steps to Add New Source Files

TODO

Modify `gamesman_install.sh` if necessary (e.g., linking new libraries as submodules.)

## Building GamesmanOne on the Savio Cluster

1. Clone GamesmanOne
    `git clone https://github.com/GamesCrafters/GamesmanOne.git`
2. Run `module load gcc/11.3.0 ucx/1.14.0 openmpi/5.0.0-ucx` if using the `savio4_htc` partition. For all other partitions, run `module load gcc openmpi`. Note that you will need to re-run one of the above `module load` commands and recompile GamesmanOne if you decided to switch to or from the `savio4_htc` partition as the modules loaded at runtime depends on the partition selected.
TODO: modify the installer script to allow USE_MPI
