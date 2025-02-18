include ../../env.mk

CC =cc
CFLAGSC=-O -I../..

INC = -I../include
LIB = -L/usr/local/lib -lmd

# include directories
INCLUDES = -I. -I../include/


# source files.
SRC =  	ioapi.c unzip.c

OBJ = $(SRC:.c=.o)


OUT = ../lib/libunzip.a


all: $(OUT)

.c.o:
	$(CC) $(INCLUDES) $(CFLAGSC) -c $< -o $@

$(OUT): $(OBJ)
	ar rcs $(OUT) $(OBJ)


clean:
	rm -f $(OBJ) $(OUT)
