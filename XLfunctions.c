#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "libraryheader.h"

#define LENGTH BUFSIZ * 1000

static struct stat buf;

// create a zip file of an excel document and extract
void createXLzip(char *s) {
  char *t;
  char *end;
  char command[BUFSIZ];
  t = replace(s, " ", "\\ ");
  s = replace(s, " ", "\\ ");
  if ((end = strstr(t, ".xls")) == NULL) {// not an excel file
    printf("Please select an valid excel file.\n");
    printf("Exiting program.\n");
    exit(0);
  }
  *end = '\0';
  snprintf(command, sizeof command, "%s%s%s%s%s", "cp ", s, " ", t, ".zip");
  system(command);

  // *******unzip zip file*******
  
  // make directory for all files
  snprintf(command, sizeof command, "%s%s", "mkdir ", t);
  if (!DIRexists(replace(t, "\\ ", " ")))
    system(command);
  else {
    snprintf(command, sizeof command, "%s%s", "rm -rf ", t);
    system(command);
  }
  // unzip in directory
  snprintf(command, sizeof command, "%s%s%s%s", "unzip -q ", t, ".zip -d ", t);
  if(system(command)) {
    printf(">>>>>>");
    printf("THE EXCEL FILE YOU ARE TRYING TO OPEN HAS A PASSWORD.\n");
    printf(">>>>>>");
    printf("REMOVE THE PASSWORD FROM THE FILE AND TRY AGAIN.\n");
    exit(1);
  }
  free(t);
  free(s);
}

/* s is the name of the cell to retrieve value (for example B11).
   fname is the name of the excel file with xls* extention.
   sheet is the number of the sheet to open,
*/
char *cell(char *s, char *fname, int sheet) {
  
  FILE *fp;
  char line[LENGTH];
  char sname[BUFSIZ]; // name of the sheet to open (xml file)
  char *begin;
  char *end;
  long int mtime_XLfile; // time excel file was last modified
  long int mtime_XLfolder; // time excel folder was last modified
  static char value[BUFSIZ]; // value of cell to return (string)

  if (stat(fname, &buf) < 0) {
    perror(fname);
    exit(1);
  }
  
  mtime_XLfile = buf.st_mtime;

  // here we remove the extension of the excel file from fname
  if ((end = strstr(fname, ".xls")) == NULL) {// not an excel file
    printf("Please select an valid excel file.\n");
    printf("Exiting program.\n");
    exit(0);
  }
  *end = '\0';

  // Check whether the the zip and corresponding directory exist
  if(!DIRexists(fname)) {
    *end = '.';
    createXLzip(fname);
    *end = '\0';
  }
  
  if (stat(fname, &buf) < 0) {
    perror(fname);
    exit(1);
  }
  
  mtime_XLfolder = buf.st_mtime;

  // unzip recently modified excel file
  if (mtime_XLfile > mtime_XLfolder) {
    *end = '.';
    createXLzip(fname);
    *end = '\0';
  }

  // create the name of the xml file to find the cell value
  snprintf(sname, sizeof sname, "%s%s%d%s", fname, "/xl/worksheets/sheet", sheet, ".xml");
  // set extension back to where it was
  *end = '.';
  if ((fp = fopen(sname, "r")) == NULL) {
    perror(sname);
    exit(1);
  }
  while (fgets(line, LENGTH, fp) != NULL) {
    if ((begin = strstr(line, s)) == NULL)
      continue;
    begin = strstr(begin, "<v>");
    begin += 3;
    end = strstr(begin, "</v>");
    *end = '\0';
    strcpy(value, begin);
    *end = '<';
    return value;
  }

  fclose(fp);
  printf("No value in cell %s, returning 0\n", s);
  return "0";
}
