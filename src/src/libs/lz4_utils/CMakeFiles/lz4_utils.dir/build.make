# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.30

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /opt/homebrew/Cellar/cmake/3.30.5/bin/cmake

# The command to remove a file.
RM = /opt/homebrew/Cellar/cmake/3.30.5/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /Users/zhengxiang/Documents/UCB/Courses/gamescrafter/Project/GamesmanOne

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /Users/zhengxiang/Documents/UCB/Courses/gamescrafter/Project/GamesmanOne/src

# Include any dependencies generated for this target.
include src/libs/lz4_utils/CMakeFiles/lz4_utils.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include src/libs/lz4_utils/CMakeFiles/lz4_utils.dir/compiler_depend.make

# Include the progress variables for this target.
include src/libs/lz4_utils/CMakeFiles/lz4_utils.dir/progress.make

# Include the compile flags for this target's objects.
include src/libs/lz4_utils/CMakeFiles/lz4_utils.dir/flags.make

src/libs/lz4_utils/CMakeFiles/lz4_utils.dir/lz4_utils.c.o: src/libs/lz4_utils/CMakeFiles/lz4_utils.dir/flags.make
src/libs/lz4_utils/CMakeFiles/lz4_utils.dir/lz4_utils.c.o: libs/lz4_utils/lz4_utils.c
src/libs/lz4_utils/CMakeFiles/lz4_utils.dir/lz4_utils.c.o: src/libs/lz4_utils/CMakeFiles/lz4_utils.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --progress-dir=/Users/zhengxiang/Documents/UCB/Courses/gamescrafter/Project/GamesmanOne/src/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object src/libs/lz4_utils/CMakeFiles/lz4_utils.dir/lz4_utils.c.o"
	cd /Users/zhengxiang/Documents/UCB/Courses/gamescrafter/Project/GamesmanOne/src/src/libs/lz4_utils && /opt/homebrew/bin/gcc-14 $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT src/libs/lz4_utils/CMakeFiles/lz4_utils.dir/lz4_utils.c.o -MF CMakeFiles/lz4_utils.dir/lz4_utils.c.o.d -o CMakeFiles/lz4_utils.dir/lz4_utils.c.o -c /Users/zhengxiang/Documents/UCB/Courses/gamescrafter/Project/GamesmanOne/src/libs/lz4_utils/lz4_utils.c

src/libs/lz4_utils/CMakeFiles/lz4_utils.dir/lz4_utils.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Preprocessing C source to CMakeFiles/lz4_utils.dir/lz4_utils.c.i"
	cd /Users/zhengxiang/Documents/UCB/Courses/gamescrafter/Project/GamesmanOne/src/src/libs/lz4_utils && /opt/homebrew/bin/gcc-14 $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /Users/zhengxiang/Documents/UCB/Courses/gamescrafter/Project/GamesmanOne/src/libs/lz4_utils/lz4_utils.c > CMakeFiles/lz4_utils.dir/lz4_utils.c.i

src/libs/lz4_utils/CMakeFiles/lz4_utils.dir/lz4_utils.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Compiling C source to assembly CMakeFiles/lz4_utils.dir/lz4_utils.c.s"
	cd /Users/zhengxiang/Documents/UCB/Courses/gamescrafter/Project/GamesmanOne/src/src/libs/lz4_utils && /opt/homebrew/bin/gcc-14 $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /Users/zhengxiang/Documents/UCB/Courses/gamescrafter/Project/GamesmanOne/src/libs/lz4_utils/lz4_utils.c -o CMakeFiles/lz4_utils.dir/lz4_utils.c.s

# Object files for target lz4_utils
lz4_utils_OBJECTS = \
"CMakeFiles/lz4_utils.dir/lz4_utils.c.o"

# External object files for target lz4_utils
lz4_utils_EXTERNAL_OBJECTS =

src/libs/lz4_utils/liblz4_utils.a: src/libs/lz4_utils/CMakeFiles/lz4_utils.dir/lz4_utils.c.o
src/libs/lz4_utils/liblz4_utils.a: src/libs/lz4_utils/CMakeFiles/lz4_utils.dir/build.make
src/libs/lz4_utils/liblz4_utils.a: src/libs/lz4_utils/CMakeFiles/lz4_utils.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --bold --progress-dir=/Users/zhengxiang/Documents/UCB/Courses/gamescrafter/Project/GamesmanOne/src/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking C static library liblz4_utils.a"
	cd /Users/zhengxiang/Documents/UCB/Courses/gamescrafter/Project/GamesmanOne/src/src/libs/lz4_utils && $(CMAKE_COMMAND) -P CMakeFiles/lz4_utils.dir/cmake_clean_target.cmake
	cd /Users/zhengxiang/Documents/UCB/Courses/gamescrafter/Project/GamesmanOne/src/src/libs/lz4_utils && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/lz4_utils.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
src/libs/lz4_utils/CMakeFiles/lz4_utils.dir/build: src/libs/lz4_utils/liblz4_utils.a
.PHONY : src/libs/lz4_utils/CMakeFiles/lz4_utils.dir/build

src/libs/lz4_utils/CMakeFiles/lz4_utils.dir/clean:
	cd /Users/zhengxiang/Documents/UCB/Courses/gamescrafter/Project/GamesmanOne/src/src/libs/lz4_utils && $(CMAKE_COMMAND) -P CMakeFiles/lz4_utils.dir/cmake_clean.cmake
.PHONY : src/libs/lz4_utils/CMakeFiles/lz4_utils.dir/clean

src/libs/lz4_utils/CMakeFiles/lz4_utils.dir/depend:
	cd /Users/zhengxiang/Documents/UCB/Courses/gamescrafter/Project/GamesmanOne/src && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /Users/zhengxiang/Documents/UCB/Courses/gamescrafter/Project/GamesmanOne /Users/zhengxiang/Documents/UCB/Courses/gamescrafter/Project/GamesmanOne/src/libs/lz4_utils /Users/zhengxiang/Documents/UCB/Courses/gamescrafter/Project/GamesmanOne/src /Users/zhengxiang/Documents/UCB/Courses/gamescrafter/Project/GamesmanOne/src/src/libs/lz4_utils /Users/zhengxiang/Documents/UCB/Courses/gamescrafter/Project/GamesmanOne/src/src/libs/lz4_utils/CMakeFiles/lz4_utils.dir/DependInfo.cmake "--color=$(COLOR)"
.PHONY : src/libs/lz4_utils/CMakeFiles/lz4_utils.dir/depend

