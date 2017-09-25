#!/bin/sh

INPUT_FILE=$1
OUTPUT_FILE=$2
OUTPUT_PARAM="output=$OUTPUT_FILE"
LOG_FILE=build_graph.log
LOG_FILE_BACKUP=$LOG_FILE.bak
MAX_LOG_SIZE=1048576

timestamp() {
	date +"%T"
}

##################################
#Prepare log file.  Make sure it exists and move it
# to backup if it's over MAX_LOG_SIZE
##################################
touch $LOG_FILE

FILESIZE=$(stat -c%s "$LOG_FILE")
if [ $FILESIZE -gt $MAX_LOG_SIZE ]
then
	mv $LOG_FILE $LOG_FILE_BACKUP
	touch $LOG_FILE
fi

echo $(timestamp) Running buildgraph.sh!  See $LOG_FILE for details.

echo "##################### buildgraph.sh #########################" >> $LOG_FILE
echo $(timestamp) Building sankey diagram from file $INPUT_FILE. >> $LOG_FILE
echo $(timestamp) Output file:  $OUTPUT_FILE >> $LOG_FILE
echo $(timestamp) Log file:  $LOG_FILE >> $LOG_FILE

##################################
#Run the parse data program which transforms input file into the binary file needed by the graph building program.
##################################

echo $(timestamp) Run parse_data.exe >> $LOG_FILE
./parse_data.exe $INPUT_FILE >> $LOG_FILE
#check return code
OUT=$?
if [ $OUT -eq 0 ]; then
        echo "$(timestamp) Parse data program completed!  Output saved to tmp.dat" |tee -a $LOG_FILE
else
        echo "$(timestamp) Parse data program failed!  See log file:  $LOG_FILE" |tee -a $LOG_FILE
	exit
fi



##################################
#Prepare the parameters file for the graph program.
##################################

touch parameters
echo input=./tmp.dat > parameters
echo $OUTPUT_PARAM >> parameters
echo phylogeny_structure_file=phylogeny_structure.txt >> parameters


##################################
#Run the graph program
##################################
echo $(timestamp) Run phylo_graph.exe >> $LOG_FILE
./phylo_graph.exe params="parameters" 2>> $LOG_FILE
#check return code
OUT=$?
if [ $OUT -eq 0 ]; then
	echo "$(timestamp) Graph program completed!  Output saved to:  $OUTPUT_FILE" |tee -a $LOG_FILE
else
	echo "$(timestamp) Graph program failed!  See log file:  $LOG_FILE" |tee -a $LOG_FILE
fi

exit
