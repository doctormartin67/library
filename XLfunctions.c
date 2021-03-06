#include "libraryheader.h"

// s is the name of the excel file to set the values of
void setXLvals(XLfile *xl, const char *s) {
    char temp[PATH_MAX + NAME_MAX + 1];
    char *pt = temp;

    strcpy(temp, s);
    strcpy(xl->fname, temp);
    if ((pt = strstr(temp, ".xls")) == NULL) // not an excel file
	errExit("[%s] [%s] not an excel file\n", __func__, s);
    *pt = '\0';
    strcpy(xl->dirname, temp);

    snprintf(temp, sizeof(temp), "%s%s", xl->dirname, "/xl/workbook.xml");
    xl->workbook = getxmlDoc(temp);

    snprintf(temp, sizeof(temp), "%s%s", xl->dirname, "/xl/sharedStrings.xml");
    xl->sharedStrings = getxmlDoc(temp);

    xl->sheetname = calloc(MAXSHEETS, sizeof(char *));
    if (xl->sheetname == NULL) errExit("[%s] calloc returned NULL\n", __func__);
    setsheetnames(xl);
    xl->sheets = calloc(MAXSHEETS, sizeof(xmlDocPtr));
    if (xl->sheets == NULL) errExit("[%s] calloc returned NULL\n", __func__);

    char **t = xl->sheetname;
    int i;
    for (i = 0; *t != NULL; i++, t++) {
	snprintf(temp, sizeof(temp), 
		"%s%s%d%s", xl->dirname, "/xl/worksheets/sheet", i + 1, ".xml");
	xl->sheets[i] = getxmlDoc(temp);
    }
    xl->sheetcnt = i;
    setnodes(xl);
}

/* fp is an open sheet, usually the sheet containing the data to evaluate 
   s is the name of the cell to retrieve value (for example B11).
   XLfile is a structure for the excel file properties.
 */
char *cell(XLfile *xl, unsigned int sheet, const char *s) {
    xmlXPathObjectPtr nodeset;
    xmlNodeSetPtr nodes;
    xmlNodePtr node;
    xmlNodePtr childnode;
    xmlChar *scell, *t;
    char *v = NULL, *row;
    int r = getrow(s);

    nodeset = xl->nodesets[sheet];
    nodes = nodeset->nodesetval;

    for (node = *nodes->nodeTab; node != NULL; node = node->next)
    {
	/* find node with row */
	row = (char *)xmlGetProp(node, (const xmlChar *)"r");
	if (atoi(row) == r)
	{
	    free(row);
	    break;
	}
	else
	    free(row);
    }
    if (node == NULL)
	return NULL;

    if ((node = node->children) == NULL)
	errExit("[%s] no value found in cell [%s]\n", __func__, s);

    for (; node != NULL; node = node->next)
    {
	/* find cell in found row */
	scell = xmlGetProp(node, (const xmlChar *)"r");
	if (xmlStrcmp(scell, (xmlChar *)s) == 0)
	{
	    xmlFree(scell);
	    break;
	}
	else
	    xmlFree(scell);
    }

    if (node == NULL || node->children == NULL)
	return NULL;

    for (childnode = node->children; childnode != NULL; childnode = childnode->next)
    {
	if (xmlStrcmp(childnode->name, (const xmlChar *)"v") == 0)
	{
	    v = (char *)xmlNodeGetContent(childnode);
	    break;
	}
    }
    if (v == NULL)
	errExit("[%s] no element <v> for cell [%s]\n", __func__, s);

    t = xmlGetProp(node, (const xmlChar *)"t");
    if (xmlStrcmp(t, (const xmlChar *)"s") == 0)
    {
	int temp = atoi(v);
	xmlFree(v);
	xmlFree(t);
	return findss(xl, temp);
    }
    else 
    {
	xmlFree(t);
	return v;
    }
    return NULL;
}

/* the excel zip has an xml file with all the string literals
   called sharedStrings.
   in the sheet xml files they are listed as a number and so we
   need to retrieve the strings given this number
 */
char *findss(XLfile *xl, int index)
{
    char *s; // value of cell to return (string)
    xmlXPathObjectPtr nodeset;
    xmlNodeSetPtr nodes;
    xmlNodePtr node;
    xmlNodePtr childnode;

    nodeset = xl->nodesetss;
    nodes = nodeset->nodesetval;
    node = nodes->nodeTab[index];

    if ((childnode = node->children) == NULL)
	errExit("[%s] The nodes have no childs, but the childs hold the string values\n", 
		__func__);

    if (!xmlStrcmp(childnode->name, (const xmlChar *)"t"))
	s = (char *)xmlNodeGetContent(childnode);
    else if (!xmlStrcmp(childnode->name, (const xmlChar *)"r"))
    {
	char temp[BUFSIZ];
	strcpy(temp, "");
	for(; childnode != NULL; childnode = childnode->next)
	{
	    xmlNodePtr gcn = childnode->children; /*grandchild node */
	    while (gcn != NULL && xmlStrcmp(gcn->name, (const xmlChar *)"t"))
		gcn = gcn->next;

	    if (gcn == NULL)
		errExit("[%s] no \"t\" element in sharedStrings.xml\n", __func__);

	    s = (char *)xmlNodeGetContent(gcn);
	    strcat(temp, s);
	    xmlFree(s);
	}
	s = strdup(temp);
    }
    else
	errExit("[%s] Unknown element [%s]\n", __func__, childnode->name);

    return s;
}

void setsheetnames(XLfile *xl)
{
    char **xls = xl->sheetname;
    xmlNodePtr p = xmlDocGetRootElement(xl->workbook);

    if (p == NULL)
	errExit("[%s] Empty document\n", __func__);

    if (xmlStrcmp(p->name, (const xmlChar *) "workbook"))
	errExit("[%s] root node != workbook\n", __func__);

    for (p = p->children; p != NULL; p = p->next)
	if ((!xmlStrcmp(p->name, (const xmlChar *)"sheets")))
	    for (xmlNodePtr ps = p->children; ps != NULL; ps = ps->next)
		*xls++ = (char *)xmlGetProp(ps, (const xmlChar *)"name");
    *xls = NULL;
}

void setnodes(XLfile *xl)
{
    xl->nodesetss = getnodeset(xl->sharedStrings, (xmlChar *)XPATHSS);
    if ((xl->nodesetss->nodesetval) == NULL)
	errExit("[%s] there are no nodes in sharedStrings.xml\n", __func__);

    for (unsigned int i = 0; i < xl->sheetcnt; i++)
    {
	xl->nodesets[i] = getnodeset(xl->sheets[i], (xmlChar *)XPATHDATA);
	if ((xl->nodesets[i]->nodesetval) == NULL)
	    errExit("[%s] there are no nodes in sheet [%s]\n", __func__,  xl->sheetname[i]);
    }
}

/* This function will return the row of a given excel cell,
   for example 11 in the case of "B11" */
unsigned int getrow(const char *cell)
{
    while (!isdigit(*cell))
	cell++;

    if (*cell == '\0')
	errExit("[%s] %s is not a valid cell, no row found\n", __func__, cell);
    return atoi(cell);
}

void nextcol(char *next)
{
    char *npt = next;
    int i = 0, j = 0, endindex = 0, finalindex = 0;
    /* endindex is the last index of the letters
       finalindex is the last index of the whole string*/

    // Find the final letter of the column.
    while (!isdigit(*(next + i)) && *(next + i) != '\0') {
	// Check if next is capital letters, if not then exit.
	if (*(next + i) <= 'z' && *(next + i) >= 'a') {
	    printf("Error: In excel columns always have capital letters and %s ", next);
	    printf("are not all capital letters. Exiting...\n");
	    exit(0);
	}
	npt = next + i++;
    }
    endindex = i--; // used to set last letter to A if all others are Z.
    while (*(next + i++))
	;
    finalindex = i; // used to shift all numbers to fit A at the end of letters
    i = endindex - 1;
    while (*npt == 'Z') {
	if (i-- == 0) {
	    while (finalindex - j++ > endindex)
		*(next + finalindex - j + 1) = *(next + finalindex - j);
	    *(next + endindex) = 'A';
	}
	*npt-- = 'A';
    }
    if (i >= 0)
	(*npt)++;
}

//---Date Functionality---

static const int commondays[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
static const int leapdays[] = {1, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

int isleapyear(int year) {
    if (!(year%4 == 0))
	return 0;
    else if (!(year%25 == 0))
	return 1;
    else if (!(year%16 == 0))
	return 0;
    else return 1;
}

void setdate(Date *date) {
    static unsigned int daytoday[BUFSIZ * 16]; // We save the searched days in this array
    static unsigned int daytomonth[BUFSIZ * 16]; // We save the searched months in this array
    static unsigned int daytoyear[BUFSIZ * 16]; // We save the searched years in this array
    unsigned int countday = 1;
    int countyear = 1900; // excel starts counting at 00/00/1900
    int currentmonth = 0;

    // If the day has already been called and saved, then we just take it from the
    // array where we save it, otherwise calculate it.
    if (!(daytoyear[date->XLday] == 0) && !(daytomonth[date->XLday] == 0)
	    && !(daytoday[date->XLday] == 0)) {
	date->day = daytoday[date->XLday];
	date->month = daytomonth[date->XLday];
	date->year = daytoyear[date->XLday];	
    }
    else {
	while (countday < date->XLday) {
	    currentmonth = currentmonth%12;
	    currentmonth++;

	    if (isleapyear(countyear))
		countday+=leapdays[currentmonth];
	    else
		countday+=commondays[currentmonth];

	    if (currentmonth == DEC && countday < date->XLday)
		countyear++;
	}
	if (isleapyear(countyear))
	    countday-=leapdays[currentmonth];
	else
	    countday-=commondays[currentmonth];

	daytoday[date->XLday] = date->XLday - countday;
	daytomonth[date->XLday] = currentmonth;
	daytoyear[date->XLday] = countyear;

	if ((date->day = daytoday[date->XLday]) > 31) {
	    printf("Error in %s: %d is not a valid day.\n", __func__, date->day);
	    exit(1);
	}
	if((date->month = daytomonth[date->XLday]) > 12) {
	    printf("Error in %s: %d is not a valid day.\n", __func__, date->month);
	    exit(1);
	}
	date->year = daytoyear[date->XLday];
    }
}

/* if XLday is 0 then this will create a date with the given day, month and year. Otherwise it
   will create it with the given XLday.*/
Date *newDate(unsigned int XLday, int year, int month, int day)
{
    int tday = day;
    int tmonth = month;
    int tyear = year;

    Date *temp = (Date *)malloc(sizeof(Date));
    if (temp == NULL) errExit("[%s] malloc returned NULL\n", __func__);

    if (tday > (isleapyear(tyear) ? leapdays[tmonth] : commondays[tmonth]))
    {
	tmonth++;
	tday = 1;
    }
    if (tmonth > DEC)
    {
	tyear++;
	tmonth = JAN;
    }

    if (XLday == 0)
    {
	temp->day = tday;
	temp->month = tmonth;
	temp->year = tyear;
    }
    else
    {
	temp->XLday = XLday;
	setdate(temp);
    }

    // Error checking
    if (temp->day > (isleapyear(temp->year) ? leapdays[temp->month] : commondays[temp->month]))
    {
	free(temp);
	return NULL;
    }
    if (temp->day < 1 || temp->day > 31)
    {
	free(temp);
	return NULL;
    }
    if (temp->month < 1 || temp->month > DEC)
    {
	free(temp);
	return NULL;
    }

    return temp;
}

Date *minDate(int argc, ...) {
    Date *min; // minimum to return
    Date *currmin; // current minimum
    va_list dates;

    va_start(dates, argc);
    min = va_arg(dates, Date *);

    for (int i = 1; i < argc; i++) {
	currmin = va_arg(dates, Date *);
	if (min->year > currmin->year) {
	    min = currmin;
	}
	else if (min->year == currmin->year) {
	    if (min->month > currmin->month) {
		min = currmin;
	    }
	    else if (min->month == currmin->month) {
		if (min->day > currmin->day)
		    min = currmin;
	    }
	}
    }
    // Error checking
    if (min->day > (isleapyear(min->year) ? leapdays[min->month] : commondays[min->month])) {
	printf("Error in newDate: %d is not a valid day of month %d\n",
		min->day, min->month);
	printf("Exiting program\n");
	exit(1);
    }
    if (min->month > DEC) {
	printf("Error in newDate: there are no %d months\n", min->month);
	printf("Exiting program\n");
	exit(1);
    }

    return min;
}

// Calculate the time in years between two dates
// m is the amount of months to subtract (usually 0 or 1)
double calcyears(Date *d1, Date *d2, int m) {
    return d2->year - d1->year + (double)(d2->month - d1->month - m)/12;
}

void printDate(Date *d) {
    if (d)
	printf("%d/%d/%d\n", d->day, d->month, d->year);
    else
	printf("(null)\n");
}
