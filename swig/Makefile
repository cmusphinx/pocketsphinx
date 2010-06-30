CFLAGS := -g -Wall -DPIC -fPIC
CPPFLAGS := -I/usr/lib/jvm/default-java/include

libtest.so: test_wrap.o
	$(CC) -shared -o $@ test_wrap.o

test_wrap.c: test.i
	$(MKDIR) -p edu/cmu/pocketsphinx
	swig -java -package edu.cmu.pocketsphinx -outdir edu/cmu/pocketsphinx test.i

clean:
	$(RM) -rf edu libtest.so test_wrap.c test_wrap.o test.class
