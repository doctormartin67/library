#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "hashtable.h"
#include "libraryheader.h"

List *lookup(char *t, char *value, Hashtable *ht) {
    List *pht;
    unsigned long hashval;
    char *key, *s, temp[strlen(t) + 1];

    if (ht->casesens)
	key = t;
    else { 
	key = temp;
	strcpy(key, t);
	upper(key);
    }

    // hash the key using the following:
    // djb2 by Dan Bernstein https://stackoverflow.com/questions/7666509/hash-function-for-string
    s = key;
    hashval = 5381;
    while (*s)
	hashval = ((hashval << 5) + hashval) + *s++; /* hashval * 33 + c */
    hashval %= ht->hashsize;


    // search item in list
    for (pht = ht->list[hashval]; pht != NULL; pht = pht->next) {
	if (strcmp(key, pht->key) == 0)
	    break;
    }

    // if the user enters NULL as value, no new value is set and item is returned
    if (value == NULL)
	return pht;

    // if key has never been set, create new entry and set key and value for new entry
    // otherwhise, remove current entry and set with new value
    if (pht == NULL) {
	pht = (List *) malloc(sizeof(*pht));
	if (pht == NULL || (pht->key = strdup(key)) == NULL) {
	    perror(__func__);
	    exit(1);
	}
	pht->next = ht->list[hashval];
	ht->list[hashval] = pht;
    }
    else
	free((void *) pht->value);
    if ((pht->value = strdup(value)) == NULL) {
	perror(__func__);
	exit(1);
    }
    return pht;
}

Hashtable *newHashtable(unsigned long n, unsigned short casesens) {
    Hashtable *ht;

    if ((ht = (Hashtable *)malloc(sizeof(Hashtable))) == NULL) {
	perror(__func__);
	exit(1);
    }
    else {
	ht->list = (List **)calloc(n, sizeof(List *));
	ht->hashsize = n;
	ht->casesens = casesens;
	return ht;
    }
}
