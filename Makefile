.PHONY: clean

scrape: scrape.c
	$(CC) -o scrape -lpcre scrape.c

clean:
	$(RM) scrape
