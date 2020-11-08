# This is a small awk file to find cells and their corresponding
# values in an excel file.
BEGIN {
    RS = "(<c r=\")|(</v></c>)";
    FS = "(<v>)|( aca=\"false\")";
}
NF == 2 {print $1 "<v>" $NF "</v>"}
NF == 3 {print $1 $2 "<v>" $NF "</v>"} #This line gets run if aca = "false" is found

