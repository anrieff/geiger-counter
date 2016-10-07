INCLUDES = -I. -I..
OPTFLAGS = -O0 -g -DDRYRUN
COMPILER = gcc

OBJECTS = mock.o tester.o pc_link.o logging.o

COMPILE = $(COMPILER) $(OPTFLAGS) $(INCLUDES) -c $< -o $@

all: dryrun

dryrun: $(OBJECTS)
	$(COMPILER) $(OPTFLAGS) -o dryrun $(OBJECTS) -lm

.c.o:
	$(COMPILE)

pc_link.o: ../pc_link.c
	$(COMPILE)

logging.o: ../logging.c
	$(COMPILE)

clean:
	-rm -f $(OBJECTS) dryrun
