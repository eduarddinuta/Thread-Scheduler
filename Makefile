CC = gcc
CFLAGS = -fPIC -Wall -lpthread
LDFLAGS = 

.PHONY: build
build: libscheduler.so

libscheduler.so: so_scheduler.o queue.o
	$(CC) $(CFLAGS) $(LDFLAGS) -shared -o $@ $^

so_scheduler.o: ../util/so_scheduler.c ../util/so_scheduler.h
	$(CC) $(CFLAGS) -o $@ -c $<

queue.o: ../util/queue.c ../util/queue.h
	$(CC) $(CFLAGS) -o $@ -c $<

.PHONY: clean
clean:
	rm -f queue.o so_scheduler.o libscheduler.so