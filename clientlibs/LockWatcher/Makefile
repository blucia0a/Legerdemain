LDMDIR=/sampa/home/blucia/cvsandbox/Legerdemain

all: LockWatcher 

LockWatcher:
	gcc LockWatcher.c -o LockWatcher.so -I$(LDMDIR) -L$(LDMDIR)/libs -fPIC -shared -g -O3 -ldl -lldm

clean:
	rm *.so
