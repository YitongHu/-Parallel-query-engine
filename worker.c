#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include "freq_list.h"
#include "worker.h"

/* The function get_word should be added to this file.
Given a word, linked-list of nodes(each node contain a word and an array of freq)
and an array of filenames. Create a array of FreqRecond that takes the given
word and search it's frequency and filename and store it in FreqRecord.
*/
FreqRecord *get_word(char *word, Node *headnode, char **filenames){
	int i;
	int j = 0;
	Node *node = headnode;
	// allocate space for an array of FreqRecord
	FreqRecord *recarray = calloc(MAXFILES + 1, sizeof(FreqRecord));
		if (recarray == NULL){
			perror("Cannot allocate anymore dynamic space");
			exit(1);
		}
	// search nodes in the linked-list of words node
	while (node != NULL){
		// when the word we want is found
		if (strcmp(word, node->word) == 0){
			// loop all its freq per file of the word
			for (i = 0; i < MAXFILES; i++){
				// set values for a FreqRecord
				if (node->freq[i] != 0){
					// set a new FreqRecord
					recarray[j].freq = node->freq[i];
					strcpy(recarray[j].filename, filenames[i]);
					j++;
				}
			}
			// set last FreqRecord to 0 frequency
			recarray[j].freq = 0;
		}
		node = node->next;
	}
	return recarray;
}

/* Sort main FreqRecord passed in by query.c from highest freq to
* lowest. Uses linear sort find the proper spot for the record we
* want to sort and push everything back.
*/
void sort_record(FreqRecord *frq, FreqRecord *mainrecords){
	int i = 0;
	int j = 1;
	FreqRecord *temp = calloc(1,sizeof(FreqRecord));
	FreqRecord *temp2 = calloc(1,sizeof(FreqRecord));
	// if it's the first element we're adding
	if (mainrecords[0].freq == 0){
		mainrecords[0] = *frq;
	}
	// main records has at least 1 record
	else{
		// loop for all elements less than max records
		while (i < MAXRECORDS){
			// when the element is less or equal than the record we
			// want to sort into the main record
			if(mainrecords[i].freq <= frq->freq){
				// temp stores the main record value and replace
				// by the one we're inserting;
				*temp = mainrecords[i];
				mainrecords[i] = *frq;
				i++;
				// an other loop to sort after the swap
				while (i != MAXRECORDS){
					// when j is odd store in temp2
					if (j%2 == 1){
						// swap records
						*temp2 = mainrecords[i];
						mainrecords[i] = *temp;
						i++;
						j++;
					}
					// when j is even store in temp
					else{
						// swap records
						*temp = mainrecords[i];
						mainrecords[i] = *temp2;
						i++;
						j++;
					}
				}
			}
			// when record we want to swap is not found
			else{
				i++;
			}
		}
	}
}

/* Print to standard output the frequency records for a word.
* Used for testing.
*/
void print_freq_records(FreqRecord *frp) {
	int i = 0;
	while(frp != NULL && frp[i].freq != 0) {
		printf("%d    %s\n", frp[i].freq, frp[i].filename);
		i++;
	}
}

/* run_worker
* - load the index found in dirname
* - read a word from the file descriptor "in"
* - find the word in the index list
* - write the frequency records to the file descriptor "out"
*/
void run_worker(char *dirname, int in, int out){
	// initialize path to filenames and index
	char listfile[PATHLENGTH];
	strcat(strcpy(listfile, dirname), "/index");
    char namefile[PATHLENGTH];
    strcat(strcpy(namefile, dirname), "/filenames");
    // create a node pointing to null
    Node *headnode = NULL;
    // create an array of filenames
    char **filenames = init_filenames();
    // create a linked-list of words
    read_list(listfile, namefile, &headnode, filenames);
	char buffer[MAXWORD];
	// while there is an input from stdin
	while(read(in, buffer, MAXWORD) > 0){
		int i;
		char alpha[62] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
		// loop to add null character after the last char of the string
		for (i = 0; i < MAXWORD; i++){
			// check when you are on the last character add a null byte
			if (strchr(alpha, buffer[i]) == NULL || i == MAXWORD-1){
				buffer[i] = '\0';
				break;
			}
		}
		FreqRecord *record = get_word(buffer, headnode, filenames);
		// loop all FreqRecord and write each and every single record to stdout
		for (i = 0; i < MAXRECORDS; i++){
			// return the FreqRecord we want to write out to stdout
			if (write(out, &record[i], sizeof(FreqRecord)) < 0){
				return;
			}
			// close the loop when the last FreqRecord is found
			if (record[i].freq == 0){
				break;
			}
		}
		// reset buffer which stores the stdin
		memset(buffer,'\0', MAXWORD);
	}	
}

