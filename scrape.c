#include <pcre.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

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

int main(int argc, char* argv[]) {
	pcre* pattern;
	const char* error;
	int errOffset;
	int result;
	int* captures;
	int captureNumber;
	char* inputBuffer;

	if (argc <= 1) {
		puts("No Pattern");
		return(EXIT_FAILURE);
	}

	pattern = pcre_compile(argv[1], 0, &error, &errOffset, NULL);

	if (pattern == NULL) {
		puts(error);
		return EXIT_FAILURE;
	}
	/* pcre_study may speed up compilation on an oft-repeated pattern */

	result = pcre_fullinfo(pattern, NULL, PCRE_INFO_CAPTURECOUNT, &captureNumber);

	/* Every capture requires 2 indecies and a scratch spot */
	/* Also, a successful match requires the same, so add 1 */
	captures = malloc(sizeof(int) * (captureNumber+1) * 3);

	/* Now, pull in the data */
	inputBuffer = allTheData(stdin, NULL, 0);

	if (inputBuffer == NULL) {
		pcre_free(pattern);
		free(captures);

		puts("Error Allocating StdIn");

		return(EXIT_FAILURE);
	}

	/* And Run the thing */
	result = pcre_exec(pattern, NULL, inputBuffer, strlen(inputBuffer), 0, 0, captures, 9);

	if (result == -1) {
		puts("Match Failed");
	}else if (result < 0) {
		printf("Error: %d\n", result);
	} else {
		puts("Match Successful");
		outputMatchCaptures(inputBuffer, captures, captureNumber);
	}

	pcre_free(pattern);
	free(captures);
	free(inputBuffer);
	return EXIT_SUCCESS;
}
