CC=$(WORK)/afl-clang-fast
ADDITIONAL="-targets=$(TMP_DIR)/BBtargets.txt -outdir=$(TMP_DIR) -flto -fuse-ld=gold -Wl,-plugin-opt=save-temps"
CFLAGS="$(ADDITIONAL)"

all: test
test: test.o
	$(CC) $(CFLAGS) -o $@ $<

test.o: test.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o test