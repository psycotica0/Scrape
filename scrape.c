#include <pcre.h>
#include <string.h>
#include <stdio.h>

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

int main(int argc, char* argv[]) {
	pcre* pattern;
	const char* error;
	int errOffset;
	int result;
	int* captures;
	int captureNumber;

	if (argc <= 2) {
		puts("Not enough arguments");
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

	result = pcre_exec(pattern, NULL, argv[2], strlen(argv[2]), 0, 0, captures, 9);

	if (result == -1) {
		puts("Match Failed");
	}else if (result < 0) {
		printf("Error: %d\n", result);
	} else {
		puts("Match Successful");
		outputMatchCaptures(argv[2], captures, captureNumber);
	}

	pcre_free(pattern);
	free(captures);
	return EXIT_SUCCESS;
}
