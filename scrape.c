#include <pcre.h>
#include <string.h>
#include <stdio.h>

int main(int argc, char* argv[]) {
	pcre *pattern;
	const char *error;
	int errOffset;
	int result;
	int captures[9];

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

	result = pcre_exec(pattern, NULL, argv[2], strlen(argv[2]), 0, 0, captures, 9);

	if (result == -1) {
		puts("Match Failed");
	}else if (result < 0) {
		printf("Error: %d\n", result);
	} else {
		puts("Match Successful");
	}

	pcre_free(pattern);
	return EXIT_SUCCESS;
}
