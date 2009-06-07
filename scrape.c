#include <pcre.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>

/* This may or may not be really gross. */
#define FREE(var) if (var != NULL) free(var);

/*
This function is used to output a single capture.

I admit I made this kind of tailored to the way I thought I was going to write this thing 

The whole match and length thing is really convenient for this, but kind of strange.
*/ 
void outputCapture(char* match, int length) {
	int i;
	putchar('{');
	for (i=0; i < length; i++) {
		putchar(match[i]);
	}
	putchar('}');
	putchar(' ');
}

/*
This function takes in a whole match and outputs each capture.
*/
void outputMatchCaptures(char* input, int* captures, int captureNumber) {
	int i;
	for (i=1; i<=captureNumber; i++) {
		int start = (2 * i);
		int end = (2 * i) + 1;

		if (start == -1 || end == -1) {
			/* This match is empty */
			outputCapture(NULL, 0);
		} else {
			outputCapture(input + captures[start], captures[end] - captures[start]);
		}
	}

	putchar('\n');
}

/*
This function returns a string which is all the data from the stream.
Simple Enough.

I think this might be kind of hacky...
*/
char* allTheData(FILE* stream, char* buffer, int size) {
	int read = 1;

	if (feof(stream)) {
		return buffer;
	}

	/* So, I'm adding 1 here to make it so the base case works... */
	buffer = realloc(buffer, sizeof(char) * (size * 2) + 1);
	if(buffer == NULL) {
		return buffer;
	}

	/* If size is 0, we'll only have room for the Null Character */
	if (size > 0) {
		/* I always want to read the next size+1 chars */
		/* But I want it to start at the null character from the last read */
		read = fread(buffer + size - 1, sizeof(char), size + 1, stream);
	}

	/* Null out the last char */
	buffer[size + read - 1] = '\0';

	return allTheData(stream, buffer, size + read);
}

void help() {
	puts("scrape [-s StartPattern] [-e EndPattern] ItemPattern\nThis program takes in stdin and an ItemPattern PCRE and lists the captures from every match of ItemPattern on the stdin of the program.\n\nOptions:\n -s StartPattern: Scrape will only start looking for ItemPattern after it's found the first occurance of StartPattern.\n -e EndPattern: Scrape will stop looking for ItemPattern after it finds EndPattern.");
}

int main(int argc, char* argv[]) {
	pcre* pattern = NULL;
	pcre* sPattern = NULL;
	pcre* ePattern = NULL;
	const char* error;
	int errOffset;
	int result;
	int* captures;
	int captureNumber;
	char* inputBuffer;
	int startOffset = 0;
	/* These Hold Pointers to the Various Patterns */
	char* startPattern = NULL;
	char* endPattern = NULL;
	char * itemPattern = NULL;
	char flag;

	while ((flag = getopt(argc, argv, "s:e:")) != -1) {
		switch (flag) {
			case 's':
				startPattern = strdup(optarg);
				break;
			case 'e':
				endPattern = strdup(optarg);
				break;
			case '?':
			default:
				help();
				return(EXIT_FAILURE);
		}
	}

	if (argc <= optind) {
		puts("No Pattern");
		help();
		FREE(startPattern);
		FREE(endPattern);
		return(EXIT_FAILURE);
	}

	itemPattern = strdup(argv[optind]);

	pattern = pcre_compile(itemPattern, 0, &error, &errOffset, NULL);

	if (pattern == NULL) {
		fprintf(stderr, "Failure in Main Pattern: %s\n",error);
		FREE(startPattern);
		FREE(endPattern);
		FREE(itemPattern);
		return EXIT_FAILURE;
	}
	/* pcre_study may speed up compilation on an oft-repeated pattern */

	/* Compile the starting pattern, if we have one */
	if (startPattern != NULL) {
		sPattern = pcre_compile(startPattern, 0, &error, &errOffset, NULL);
		if (sPattern == NULL) {
			fprintf(stderr, "Failure in Starting Pattern: %s\n",error);
			pcre_free(pattern);
			FREE(startPattern);
			FREE(endPattern);
			FREE(itemPattern);
			return EXIT_FAILURE;
		}
	}

	/* Compile the ending pattern, if we have one */
	if (endPattern != NULL) {
		ePattern = pcre_compile(endPattern, 0, &error, &errOffset, NULL);
		if (ePattern == NULL) {
			fprintf(stderr, "Failure in Ending Pattern: %s\n",error);
			pcre_free(pattern);
			if (sPattern != NULL) pcre_free(sPattern);
			FREE(startPattern);
			FREE(endPattern);
			FREE(itemPattern);
			return EXIT_FAILURE;
		}
	}

	result = pcre_fullinfo(pattern, NULL, PCRE_INFO_CAPTURECOUNT, &captureNumber);

	/* Every capture requires 2 indecies and a scratch spot */
	/* Also, a successful match requires the same, so add 1 */
	captures = malloc(sizeof(int) * (captureNumber+1) * 3);

	/* Now, pull in the data */
	inputBuffer = allTheData(stdin, NULL, 0);

	if (inputBuffer == NULL) {
		pcre_free(pattern);
		if (sPattern != NULL) pcre_free(sPattern);
		if (ePattern != NULL) pcre_free(ePattern);
		free(captures);
		FREE(startPattern);
		FREE(endPattern);
		FREE(itemPattern);

		puts("Error Allocating StdIn");

		return(EXIT_FAILURE);
	}

	/* Find the starting offset */
	if (sPattern != NULL) {
		result = pcre_exec(sPattern, NULL, inputBuffer, strlen(inputBuffer), 0, 0, captures, (captureNumber+1)*3);
		if (result >= 0) {
			startOffset = captures[1];
		}
	}

	/* Find the ending */
	if (ePattern != NULL) {
		result = pcre_exec(ePattern, NULL, inputBuffer, strlen(inputBuffer), startOffset, 0, captures, (captureNumber+1)*3);
		if (result >= 0) {
			/* If we've found the end pattern, just close off the input buffer there */
			inputBuffer[captures[0]] = '\0';
		}
	}

	do {
		/* And Run the thing */
		result = pcre_exec(pattern, NULL, inputBuffer, strlen(inputBuffer), startOffset, 0, captures, (captureNumber+1)*3);

		if (result >= 0) {
			outputMatchCaptures(inputBuffer, captures, captureNumber);
			/* Update the start to start searching again after the end of the first match */
			startOffset = captures[1];
		}
	} while (result >= 0);

	pcre_free(pattern);
	if (sPattern != NULL) pcre_free(sPattern);
	if (ePattern != NULL) pcre_free(ePattern);
	free(captures);
	free(inputBuffer);
	FREE(startPattern);
	FREE(endPattern);
	FREE(itemPattern);

	return EXIT_SUCCESS;
}
