#include <pcre.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>

#define CLEAN \
if (pattern != NULL) pcre_free(pattern);\
if (sPattern != NULL) pcre_free(sPattern);\
if (ePattern != NULL) pcre_free(ePattern);\
if (cPattern != NULL) pcre_free(cPattern);\
if (studied != NULL) free(studied);\
if (captures != NULL) free(captures);\
if (inputBuffer != NULL) free(inputBuffer);\
if (startPattern != NULL) free(startPattern);\
if (endPattern != NULL) free(endPattern);\
if (itemPattern != NULL) free(itemPattern);\
if (before != NULL) free(before);\
if (after != NULL) free(after);\
if (separator != NULL) free(separator);\

/*
This function is used to output a single capture.

I admit I made this kind of tailored to the way I thought I was going to write this thing 

The whole match and length thing is really convenient for this, but kind of strange.
*/ 
void outputCapture(char* match, int length, char* before, char* after) {
	int i;
	/* I've used fputs because it doesn't append a newline */
	fputs(before, stdout);
	for (i=0; i < length; i++) {
		putchar(match[i]);
	}
	fputs(after, stdout);
}

/*
This function takes in a whole match and outputs each capture.
*/
void outputMatchCaptures(char* input, int* captures, int captureNumber, char* before, char* after, char * separator) {
	int i;
	for (i=1; i<=captureNumber; i++) {
		int start = (2 * i);
		int end = (2 * i) + 1;

		if (start == -1 || end == -1) {
			/* This match is empty */
			outputCapture(NULL, 0, before, after);
		} else {
			outputCapture(input + captures[start], captures[end] - captures[start], before, after);
		}

		if (i+1 <= captureNumber) {
			/* This isn't the last one */
			fputs(separator, stdout);
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
	puts("scrape [options] ItemPattern\nThis program takes in stdin and an ItemPattern PCRE and lists the captures from every match of ItemPattern on the stdin of the program.\n\nOptions:\n -s StartPattern: Scrape will only start looking for ItemPattern after it's found the first occurance of StartPattern.\n -e EndPattern: Scrape will stop looking for ItemPattern after it finds EndPattern.\n -c ContinuePattern: Scrape will output the first capture of this pattern to stderr\n  This is inteded to be used to capture the location of the next item in a paginated output\n -b String: The given String will be put before every field in the output\n  Default:'{'\n -a String: The given String will be put after every field in the output\n  Default:'}'\n -d String: The given String will be put between fields in the output\n  Default:' '");
}

int main(int argc, char* argv[]) {
	pcre* pattern = NULL;
	pcre* sPattern = NULL;
	pcre* ePattern = NULL;
	pcre* cPattern = NULL;
	pcre_extra* studied = NULL;
	const char* error;
	int errOffset;
	int result;
	int* captures = NULL;
	int captureNumber;
	char* inputBuffer = NULL;
	int inputBufferSize;
	int startOffset = 0;
	/* These Hold Pointers to the Various Patterns */
	char* startPattern = NULL;
	char* endPattern = NULL;
	char* itemPattern = NULL;
	char* continuePattern = NULL;
	char flag;
	/* These hold the various delimiters */
	char* before = NULL;
	char* after = NULL;
	char* separator = NULL;

	while ((flag = getopt(argc, argv, "s:e:c:b:a:d:")) != -1) {
		switch (flag) {
			case 's':
				startPattern = strdup(optarg);
				break;
			case 'e':
				endPattern = strdup(optarg);
				break;
			case 'c':
				continuePattern = strdup(optarg);
				break;
			case 'a':
				after = strdup(optarg);
				break;
			case 'b':
				before = strdup(optarg);
				break;
			case 'd':
				separator = strdup(optarg);
				break;
			case '?':
			default:
				help();
				/* This will probably not do anything, but I'll put it here incase it does at some point */
				CLEAN;
				return(EXIT_FAILURE);
		}
	}

	if (argc <= optind) {
		puts("No Pattern");
		help();
		CLEAN;
		return(EXIT_FAILURE);
	}

	itemPattern = strdup(argv[optind]);

	pattern = pcre_compile(itemPattern, 0, &error, &errOffset, NULL);

	if (pattern == NULL) {
		fprintf(stderr, "Failure in Main Pattern: %s\n",error);
		CLEAN;
		return EXIT_FAILURE;
	}

	/* pcre_study may speed up execution of an oft-repeated pattern */
	studied = pcre_study(pattern, 0, &error);
	/* If it fails, I don't really care. */

	/* Compile the starting pattern, if we have one */
	if (startPattern != NULL) {
		sPattern = pcre_compile(startPattern, 0, &error, &errOffset, NULL);
		if (sPattern == NULL) {
			fprintf(stderr, "Failure in Starting Pattern: %s\n",error);
			CLEAN;
			return EXIT_FAILURE;
		}
	}

	/* Compile the ending pattern, if we have one */
	if (endPattern != NULL) {
		ePattern = pcre_compile(endPattern, 0, &error, &errOffset, NULL);
		if (ePattern == NULL) {
			fprintf(stderr, "Failure in Ending Pattern: %s\n",error);
			CLEAN;
			return EXIT_FAILURE;
		}
	}

	/* Compile the continuing pattern, if we have one */
	if (continuePattern != NULL) {
		cPattern = pcre_compile(continuePattern, 0, &error, &errOffset, NULL);
		if (cPattern == NULL) {
			fprintf(stderr, "Failure in Continuing Pattern: %s\n",error);
			CLEAN;
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
		puts("Error Allocating StdIn");
		CLEAN;
		return(EXIT_FAILURE);
	}

	inputBufferSize = strlen(inputBuffer);

	/* Find the starting offset */
	if (sPattern != NULL) {
		result = pcre_exec(sPattern, NULL, inputBuffer, inputBufferSize, 0, 0, captures, (captureNumber+1)*3);
		if (result >= 0) {
			startOffset = captures[1];
		}
	}

	/* Find the ending */
	if (ePattern != NULL) {
		result = pcre_exec(ePattern, NULL, inputBuffer, inputBufferSize, startOffset, 0, captures, (captureNumber+1)*3);
		if (result >= 0) {
			/* If we've found the end pattern, just close off the input buffer there */
			inputBuffer[captures[0]] = '\0';
			/* Now we need to recompute the size */
			inputBufferSize = strlen(inputBuffer);
		}
	}

	/* Find the Continuation */
	if (cPattern != NULL) {
		result = pcre_exec(cPattern, NULL, inputBuffer, inputBufferSize, startOffset, 0, captures, (captureNumber+1)*3);
		if (result >= 1) {
			int i;
			/* If we've found the end pattern, spit the first capture to stderr */
			for (i=captures[2]; i<captures[3]; i++) {
				fputc(inputBuffer[i], stderr);
			}
		}
	}

	/* If we don't have any separators yet, us the standard */
	if (before == NULL) before = strdup("{");
	if (after == NULL) after = strdup("}");
	if (separator == NULL) separator = strdup(" ");

	do {
		/* And Run the thing */
		result = pcre_exec(pattern, studied, inputBuffer, inputBufferSize, startOffset, 0, captures, (captureNumber+1)*3);

		if (result >= 0) {
			outputMatchCaptures(inputBuffer, captures, captureNumber, before, after, separator);
			/* Update the start to start searching again after the end of the first match */
			startOffset = captures[1];
		}
	} while (result >= 0);

	CLEAN;
	return EXIT_SUCCESS;
}
