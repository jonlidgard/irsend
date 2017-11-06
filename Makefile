.PHONY: clean all
all: irsend
clean:
	rm -f irsendpru_bin.h irsend
irsendpru_bin.h: Makefile irsendpru.p
	pasm -c -CprussPru0Code irsendpru.p irsendpru
irsend: Makefile irsend.c irsendpru_bin.h
	gcc -o irsend -g -l prussdrv -l config irsend.c

