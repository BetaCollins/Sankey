Summary:

This is a pair of programs and a shell script used to build sankey diagrams from R_16 read data.
The graphing program was built by Andreas Sundquist.
The r_16 read data parsing program, makefile, and shell script were built by Matthew Davis.

----------------------------
Compiling the programs
----------------------------
** package dependencies - these are packages I had to get on a fresh Linux Mint 17.3 install to build the graph program **
libcairo2-dev  (this is the lib used to draw the graph)
libbz2-dev  (for bzlib.h)
zlib1g-dev  (for zlib.h)

To build in release mode:
make

To build in debug:
make DEBUG=1

To remove build files (.o):
make clean

To remove all build files and built executables:
make dist-clean

Once built, the executables phylo_graph.exe and parse_data.exe should be present


-------------------------------
Running everything
--------------------------------
Building the sankey graph from the input data consists of running 2 programs.  However, a shell script is supplied which will run the two progams and manage logging and parameters.  

First make sure the script is executable:
chmod +x buildgraph.sh

To run using the shell script:
./buildgraph.sh [INPUT_FILE] [OUTPUT_FILE]

For example, to read the data from file "data/AHH16599_raw-table.txt" and write the graph to "AHH16559.png", run the command:
./buildgraph.sh data/AHH16599_raw-table.txt AHH16559.png


--------------------------------
Parse data program.  parse_data.exe
--------------------------------
command syntax:  parse_data.exe [INPUT_FILE]

his is an original program built to transform the R16 read data to a format consumable by the graph program.

This program takes as input a tab delimited file containing R16 reads.  Each row corresponds to a read, and the required columns for a row are:
column 0 - unused
column 1 - value, percentage of reads
column 2 - unused
column 3 - unused
column 4 - kingdom label in the format "k:[KINGDOM_NAME]"
column 5 - phylum label in the format "p:[PHYLUMN_NAME]"
column 6 - class label in the format "p:[CLASS_NAME]"
column 7 - order label in the format "p:[ORDER_NAME]"
column 8 - family label in the format "p:[FAMILY_NAME]"
column 9 - genus label in the format "p:[GENUS_NAME]"
column 10 - species label in the format "p:[SPECIES_NAME]"

*note that the unused columns must exist since the column indices are hard-coded in the program.

This parses the input read file and transforms the data into the GraphPhylogeny program's custom data structures.  It then serializes these data structures into a binary file which will be read by the graph building program.  The serialized data is stored in the file ./tmp.dat


-------------------------------------
GraphPhylogeny program  phylo_graph.exe
-------------------------------------
command syntax:  phylo_graph.exe params="parameters"

Instead of operating purely from command line parameters, the phylo_graph program takes an input file ("parameters" in the above example command) which contains the following parameters:

input=./tmp.dat
output=AHH16559.png
phylogeny_structure_file=phylogeny_structure.txt

Parameter descriptions:
input:  (./tmp.dat) this is the binary file with the classification data.  tmp.dat is the name of the file parse_data.exe creates.
output:  This is the name of the file where the graph program will write the png.
phylogeny_structure_file:  defines the labels for the phylogeny structure.  I do not know what will happen if you use a different file than the phylogeny_structure.txt which came with the graph program.

The program reads the input file and uses the data and the Cairo library to draw a sankey diagram.  That diagram is written to the filename given in the output parameter.


---------------------------------------
buildgraph.sh
---------------------------------------
command syntax:  ./buildgraph.sh [INPUT_FILE] [OUTPUT_FILE]

This shell script does the following:
1.  executes the parse data program with the INPUT_FILE parameter
2.  writes out the parameters file needed by the graph program, including OUTPUT_FILE as the output png.
3.  executes the graph program with the parameters file which results in OUTPUT_FILE being produced (the final graph.)
The script also:
Manages logging:  creates a log file (build_graph.log) redirects all of the programs' output into it.  It also will roll the log file to a backup if the size goes over a certain threshold (currently set at 1MB but can be changed in the script).
Tracks return codes:  Will let the user know if the programs succeed or fail and direct them to the log file.
