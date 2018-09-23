#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include "freq_list.h"
#include "worker.h"
#include <sys/wait.h>

int main(int argc, char **argv) {
	
	char ch;
	char path[PATHLENGTH];
	char *startdir = ".";
	int subdir = 0;
	// initialize array to store pipe we want to write and
	// read to between parent and childs
	int arraypwrite[MAXFILES];
	int arraycread[MAXFILES];
	// set string to read stdin
	char buffer[MAXWORD];

	//check passed input
	while((ch = getopt(argc, argv, "d:")) != -1) {
		switch (ch) {
			case 'd':
			startdir = optarg;
			break;
			default:
			fprintf(stderr, "Usage: query [-d DIRECTORY_NAME]\n");
			exit(1);
		}
	}
	
	// Check if you are on a directory that can be accessed
	DIR *dirp;
	if((dirp = opendir(startdir)) == NULL) {
		perror("opendir");
		exit(1);
	} 

	/* For each entry in the directory, eliminate . and .., and check
	* to make sure that the entry is a directory, then call run_worker
	* to process the index file contained in the directory.
	* Note that this implementation of the query engine iterates
	* sequentially through the directories, and will expect to read
	* a word from standard input for each index it checks.
	*/
	struct dirent *dp;
	while((dp = readdir(dirp)) != NULL) {

		if(strcmp(dp->d_name, ".") == 0 || 
		   strcmp(dp->d_name, "..") == 0 ||
		   strcmp(dp->d_name, ".svn") == 0){
			continue;
		}
		strncpy(path, startdir, PATHLENGTH);
		strncat(path, "/", PATHLENGTH - strlen(path) - 1);
		strncat(path, dp->d_name, PATHLENGTH - strlen(path) - 1);

		struct stat sbuf;
		if(stat(path, &sbuf) == -1) {
			// This should only fail if we got the path wrong
			// or we don't have permissions on this entry.
			perror("stat");
			exit(1);
		} 

		// Only fork and pipe if it is a directory, otherwise ignore it.
		if(S_ISDIR(sbuf.st_mode)) {
			// create 2 pipes to store info send between child and parent
			int ptoc[2];
			int ctop[2];
			if(pipe(ptoc) == -1) { 
				perror("pipe cannot be made"); 
				exit(1);
			}
			if (pipe(ctop) == -1) {
				perror("pipe cannot be made"); 
				exit(1);
			}
			//create processes
			pid_t childpid = fork();
			// when it's the child process
			if (childpid == 0){
				// close parent write to child and child read from parent
				close(ptoc[1]);
				close(ctop[0]);
				// run_worker on pipe, parent read from child and child write to parent
				run_worker(path, ptoc[0], ctop[1]);
				exit(0);
			}
			// when there is an error
			else if (childpid < 0){
				perror("No child can be made(forking failed)");
				exit(-1);
			}
			// when it's the parent process
			else{
				// parent write to child and child read from parent are stored
				arraypwrite[subdir] = ptoc[1];
				arraycread[subdir] = ctop[0];
				// close parent read from child and child write to parent
				close(ptoc[0]);
				close(ctop[1]);
			}
			subdir++;
		}
	}
	
	int i;
	// get input from user
	while (read(STDIN_FILENO, buffer, MAXWORD) > 0){
		// initialize varibles for loops and allocate space for records
		int j = 0;
		int running = 1;
		int dir[subdir];
		FreqRecord *mainrecord = calloc(MAXRECORDS + 1, sizeof(FreqRecord));
		if (mainrecord == NULL){
			perror("No space available to store FreqRecord array");
			exit(1);
		}
		FreqRecord *frq = calloc(1, sizeof(FreqRecord));
		if (frq == NULL){
			perror("No space available to store FreqRecord");
			exit(1);
		}
		
		// send user input into every pipe and populate array
		// that track reading from children process
		for (i = 0; i < subdir; i++){
			write(arraypwrite[i], buffer, MAXWORD);
			dir[i] = 0;
		}
		// loop until everything has been outputed
		while (running != 0){
			running = 0;
			// loop through directories
			for (i = 0; i < subdir; i++){
				if (dir[i] != 1){
					// read what processes returns
					if(read(arraycread[i], frq, sizeof(FreqRecord)) > 0){
						// when is not the last record, sort it then keep running
						// from read of a single directory
						if(frq->freq != 0){
							sort_record(frq, mainrecord);
							// free space and re allocate the single record
							free(frq);
							FreqRecord *frq = calloc(1, sizeof(FreqRecord));
							if (frq == NULL){
								perror("No space available to store FreqRecord");
								exit(1);
							}
							running = 1;
							j++;
						}
						// when it's the last one pass to next directory
						else{
							dir[i] = 1;
							continue;
						
						}
					}
					// set the last record to be 0 when max is reached
					if(j >= MAXRECORDS){
						mainrecord[j].freq = 0;
					}
				}
			}
			
			
		}
		print_freq_records(mainrecord);
		// free and re allocate space for max record for next input
		free(mainrecord);
		mainrecord = calloc(MAXRECORDS + 1, sizeof(FreqRecord));
		if (mainrecord == NULL){
			perror("No space available to store FreqRecord array");
			exit(1);
		}
	}
	//close pipe that were not closed when everything ends
	for (i = 0; i < subdir; i++){
		close(arraypwrite[i]);
		close(arraycread[i]);
	}
	// wait for children process to finish
	while (wait(NULL) > 0){
		if (wait(NULL) < 0){
			continue;
		}
	}
	return 0;
}


