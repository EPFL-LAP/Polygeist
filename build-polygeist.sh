#!/bin/bash

# Kill the whole script on Ctrl+C
trap "exit" INT

# Directory where the script is being ran from (must be directory where script
# is located!)
SCRIPT_CWD=$PWD

#### Helper functions ####

# Helper function to print large section title text
echo_section() {
    echo ""
    echo "# ===----------------------------------------------------------------------=== #"
    echo "# $1"
    echo "# ===----------------------------------------------------------------------=== #"
    echo ""
}

# Helper function to print subsection title text
echo_subsection() {
    echo -e "\n# ===--- $1 ---==="
}

# Helper function to exit script on failed command
exit_on_fail() {
    if [[ $? -ne 0 ]]; then
        if [[ ! -z $1 ]]; then
            echo -e "\n$1"
        fi
        echo_subsection "Build failed!"
        exit 1
    fi
}

# Prepares to build a particular part of the project by creating a "build"
# folder for it, cd-ing to it, and displaying a message to stdout.
# $1: Name of the project part
# $2: Path to build folder (relative to script's CWD)
prepare_to_build_project() {
  local project_name=$1
  local build_dir=$2
  cd "$SCRIPT_CWD" && mkdir -p "$build_dir" && cd "$build_dir"
  echo_section "Building $project_name ($build_dir)"
}

# Create symbolic link from the bin/ directory to an executable file built by
# the repository. The symbolic link's name is the same as the executable file.
# The path to the executable file must be passed as the first argument to this
# function and be relative to the repository's root. The function assumes that
# the bin/ directory exists and that the current working directory is the
# repository's root.
create_symlink() {
    local src=$1
    local dst="bin/$(basename $1)"
    echo "$dst -> $src"
    ln -f --symbolic $src $dst
}

# Same as create_symlink but creates the symbolic link inside the bin/generators
# subfolder.
create_generator_symlink() {
    local src=$1
    local dst="bin/generators/$(basename $1)"
    echo "$dst -> $src"
    ln -f --symbolic ../../$src $dst
}


# Determine whether cmake should be re-configured by looking for a
# CMakeCache.txt file in the current working directory.
should_run_cmake() {
  if [[ -f "CMakeCache.txt" && $FORCE_CMAKE -eq 0 ]]; then
    echo "CMake configuration found, will not re-configure cmake"
    echo "Run script with -f or --force flag to re-configure cmake"
    echo ""
    return 1
  fi
  return 0
}

# Run ninja using the number of threads provided as argument, if any. Otherwise,
# let ninja pick the number of threads to use
run_ninja() {
  if [[ $NUM_THREADS -eq 0 ]]; then
    ninja
  else
    ninja -j "$NUM_THREADS"
  fi
}

#### Parse arguments ####

CMAKE_COMPILERS="-DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++"
CMAKE_LLVM_BUILD_OPTIMIZATIONS="-DLLVM_CCACHE_BUILD=ON -DLLVM_USE_LINKER=lld"
CMAKE_POLYGEIST_BUILD_OPTIMIZATIONS="-DPOLYGEIST_USE_LINKER=lld"
CMAKE_COMPILERS="-DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++"
CMAKE_LLVM_BUILD_OPTIMIZATIONS="-DLLVM_CCACHE_BUILD=ON -DLLVM_USE_LINKER=lld"
CMAKE_POLYGEIST_BUILD_OPTIMIZATIONS="-DPOLYGEIST_USE_LINKER=lld"
CMAKE_DYNAMATIC_BUILD_OPTIMIZATIONS="-DDYNAMATIC_CCACHE_BUILD=ON -DLLVM_USE_LINKER=lld"
CMAKE_DYNAMATIC_ENABLE_XLS=""
CMAKE_DYNAMATIC_ENABLE_LEQ_BINARIES=""
ENABLE_TESTS=0
FORCE_CMAKE=0
NUM_THREADS=0
LLVM_PARALLEL_LINK_JOBS=2
BUILD_TYPE="Debug"

# Loop over command line arguments and update script variables
PARSE_ARG=""
for arg in "$@";
do
    if [[ $PARSE_ARG == "num-threads" ]]; then
      NUM_THREADS="$arg"
      PARSE_ARG=""
    elif [[ $PARSE_ARG == "llvm-parallel-link-jobs" ]]; then
      LLVM_PARALLEL_LINK_JOBS="$arg"
      PARSE_ARG=""
    else
      case "$arg" in
          "--disable-build-opt" | "-o")
              CMAKE_COMPILERS=""
              CMAKE_LLVM_BUILD_OPTIMIZATIONS=""
              CMAKE_POLYGEIST_BUILD_OPTIMIZATIONS=""
              CMAKE_DYNAMATIC_BUILD_OPTIMIZATIONS=""
              ;;
          "--force" | "-f")
              FORCE_CMAKE=1
              ;;
          "--release" | "-r")
              BUILD_TYPE="Release"
              ;;
          "--check" | "-c")
              ENABLE_TESTS=1
              ;;
          "--threads" | "-t")
              PARSE_ARG="num-threads"
              ;;
          "--llvm-parallel-link-jobs")
              PARSE_ARG="llvm-parallel-link-jobs"
              ;;
          "--help" | "-h")
              print_help_and_exit
              ;;
          *)
              echo "Unknown argument \"$arg\""
              ;;
      esac
    fi
done
if [[ $PARSE_ARG != "" ]]; then
  echo "Missing argument \"$PARSE_ARG\""
  print_help_and_exit
fi

#### Polygeist ####

prepare_to_build_project "LLVM" "llvm-project/build"

# CMake
if should_run_cmake ; then
  cmake -G Ninja ../llvm \
      -DLLVM_ENABLE_PROJECTS="mlir;clang" \
      -DLLVM_TARGETS_TO_BUILD="host" \
      -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
      -DLLVM_PARALLEL_LINK_JOBS=$LLVM_PARALLEL_LINK_JOBS \
      $CMAKE_COMPILERS $CMAKE_LLVM_BUILD_OPTIMIZATIONS
  exit_on_fail "Failed to cmake polygeist/llvm-project"
fi

# Build
run_ninja
exit_on_fail "Failed to build polygeist/llvm-project"
if [[ ENABLE_TESTS -eq 1 ]]; then
    ninja check-mlir
    exit_on_fail "Tests for polygeist/llvm-project failed"
fi

prepare_to_build_project "Polygeist" "build"

# CMake
if should_run_cmake ; then
  cmake -G Ninja .. \
      -DMLIR_DIR=$PWD/../llvm-project/build/lib/cmake/mlir \
      -DCLANG_DIR=$PWD/../llvm-project/build/lib/cmake/clang \
      -DLLVM_TARGETS_TO_BUILD="host" \
      -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
      $CMAKE_COMPILERS $CMAKE_POLYGEIST_BUILD_OPTIMIZATIONS
  exit_on_fail "Failed to cmake polygeist"
fi

# Build
run_ninja
exit_on_fail "Failed to build polygeist"
if [[ ENABLE_TESTS -eq 1 ]]; then
    ninja check-polygeist-opt
    exit_on_fail "Tests for polygeist failed"
    ninja check-cgeist
    exit_on_fail "Tests for polygeist failed"
fi

echo_subsection "Build successful!"
