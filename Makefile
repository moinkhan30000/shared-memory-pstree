CC	:= gcc
CFLAGS := -g -Wall

TARGETS := app1-add app1-get app2-insert app2-get app3-insert-delete app3-get app4-mixed libpstree.a

all: $(TARGETS)

# PSTree library
PST_SRC := pstree.c
PST_OBJS := $(PST_SRC:.c=.o)

libpstree.a: $(PST_OBJS)
	ar rcs $@ $(PST_OBJS)

PST_LIB := -L . -lpstree

# Generic rule for compiling any .c to .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# App targets
app1-add: app1-add.o libpstree.a
	$(CC) $(CFLAGS) -o $@ app1-add.o $(PST_LIB)

app1-get: app1-get.o libpstree.a
	$(CC) $(CFLAGS) -o $@ app1-get.o $(PST_LIB)

app2-insert: app2-insert.o libpstree.a
	$(CC) $(CFLAGS) -o $@ app2-insert.o $(PST_LIB)

app2-get: app2-get.o libpstree.a
	$(CC) $(CFLAGS) -o $@ app2-get.o $(PST_LIB)

app3-insert-delete: app3-insert-delete.o libpstree.a
	$(CC) $(CFLAGS) -o $@ app3-insert-delete.o $(PST_LIB)

app3-get: app3-get.o libpstree.a
	$(CC) $(CFLAGS) -o $@ app3-get.o $(PST_LIB)

app4-mixed: app4-mixed.o libpstree.a
	$(CC) $(CFLAGS) -o $@ app4-mixed.o $(PST_LIB)

# Clean
clean:
	rm -rf core *.o *.out *~ $(TARGETS) libpstree.a
