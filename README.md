# QCIRSYM - A Tool for finding symmetries in QCIR formulas in cleansed-prenex form

Developer: Tom Peham

## Overview
Symmetry breaking is a well established method in SAT-solving. Although symmetry breaking techniques for SAT-solving have been applied to QBF reasoning only QBF with a CNF matrix have been considered so far. Recent works have explored symmetries in QBF more thoroughly. In contrast to SAT, for QBF two kinds of symmetry breakers have to be considered: existential and universal. The proposed symmetry breaking predicates for QBF are problematic for these formulas because the introduction of universal symmetry breakers might destroy the CNF structure of the underlying matrix. One way to avoid the issue is to work in a format that doesn't require the CNF format. Formats such as QCIR (described in [[1]](http://www.qbflib.org/qcir.pdf)) allow for the encoding of arbitrary QBF. QCIRSYM is a tool that detects symmetries in QCIR formulas in cleansed prenex format.

If you have any questions, feel free to contact me via pehamtom@gmx.at or by creating an issue on GitHub.

## Usage

### System Requirements

The package has been tested under Linux (Ubuntu 18.04, 64-bit) and should be compatible with any current version of g++ supporting C++11 and a minimum CMake version of 3.13.
The project also requires the Saucy graph automorphism tool [[2]](http://vlsicad.eecs.umich.edu/BK/SAUCY/). The tool has been tested with saucy version 3.0.

### Build and Run

To build the tool go to the project folder and execute the following:

1) Configure CMake
    ```commandline
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    ```
2) Build the respective target.
    ```commandline
   cmake --build ./build --config Release
   ```
Make sure that the files "saucy.h" and "saucy.c" are in a folder called "saucy/". The "saucy/" folder should be on the topmost level of the project folder.

To use the tool go to the build directory and run 

    ./qcirsym <input file>
    
 ### Output Format
 The output is given as a list of orbits seperated by linebreaks. These orbits form the generating set of a symmetry group of the QBF. As an example consider the QCIR-file test.qcir in the examples-folder. Running QCIR on this file gives the following output
 ```
 (3 4)(-3 -4)
 (1 2)(-1 -2)
 ```
 This means that swapping 3 with 4 and swapping -3 with -4 is a symmetry of the formula. The same is true for the variables 1 and 2.
