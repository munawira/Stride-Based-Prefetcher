#
#
CC=g++
CFLAGS=-g

# comment line below for Linux machines
all: cache
cache:	
	$(CC) $(CFLAGS) -o sim_cache sim_cache.cc 

clean:
	\rm -f sim_cache


