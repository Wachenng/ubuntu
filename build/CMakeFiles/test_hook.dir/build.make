# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.22

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

# Produce verbose output by default.
VERBOSE = 1

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
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/ubuntu/server

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/ubuntu/server/build

# Include any dependencies generated for this target.
include CMakeFiles/test_hook.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/test_hook.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/test_hook.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/test_hook.dir/flags.make

CMakeFiles/test_hook.dir/tests/test_hook.cc.o: CMakeFiles/test_hook.dir/flags.make
CMakeFiles/test_hook.dir/tests/test_hook.cc.o: ../tests/test_hook.cc
CMakeFiles/test_hook.dir/tests/test_hook.cc.o: CMakeFiles/test_hook.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/ubuntu/server/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/test_hook.dir/tests/test_hook.cc.o"
	/usr/bin/c++ $(CXX_DEFINES) -D__FILE__=\"tests/test_hook.cc\" $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/test_hook.dir/tests/test_hook.cc.o -MF CMakeFiles/test_hook.dir/tests/test_hook.cc.o.d -o CMakeFiles/test_hook.dir/tests/test_hook.cc.o -c /home/ubuntu/server/tests/test_hook.cc

CMakeFiles/test_hook.dir/tests/test_hook.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/test_hook.dir/tests/test_hook.cc.i"
	/usr/bin/c++ $(CXX_DEFINES) -D__FILE__=\"tests/test_hook.cc\" $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/ubuntu/server/tests/test_hook.cc > CMakeFiles/test_hook.dir/tests/test_hook.cc.i

CMakeFiles/test_hook.dir/tests/test_hook.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/test_hook.dir/tests/test_hook.cc.s"
	/usr/bin/c++ $(CXX_DEFINES) -D__FILE__=\"tests/test_hook.cc\" $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/ubuntu/server/tests/test_hook.cc -o CMakeFiles/test_hook.dir/tests/test_hook.cc.s

# Object files for target test_hook
test_hook_OBJECTS = \
"CMakeFiles/test_hook.dir/tests/test_hook.cc.o"

# External object files for target test_hook
test_hook_EXTERNAL_OBJECTS =

../bin/test_hook: CMakeFiles/test_hook.dir/tests/test_hook.cc.o
../bin/test_hook: CMakeFiles/test_hook.dir/build.make
../bin/test_hook: ../lib/libsylar.so
../bin/test_hook: CMakeFiles/test_hook.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/ubuntu/server/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable ../bin/test_hook"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/test_hook.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/test_hook.dir/build: ../bin/test_hook
.PHONY : CMakeFiles/test_hook.dir/build

CMakeFiles/test_hook.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/test_hook.dir/cmake_clean.cmake
.PHONY : CMakeFiles/test_hook.dir/clean

CMakeFiles/test_hook.dir/depend:
	cd /home/ubuntu/server/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/ubuntu/server /home/ubuntu/server /home/ubuntu/server/build /home/ubuntu/server/build /home/ubuntu/server/build/CMakeFiles/test_hook.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/test_hook.dir/depend

