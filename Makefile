.PHONY: clean

scrape: scrape.c
	$(CC) -o scrape -g -lpcre scrape.c

clean:
	$(RM) scrape
