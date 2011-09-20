DRPDIR=/sampa/home/blucia/cvsandbox/DRProbe/drprobe/
LDMDIR=/sampa/home/blucia/cvsandbox/Legerdemain

all: ConcurrentWatcher 

ConcurrentWatcher:
	gcc ConcurrentWatcher.c -o ConcurrentWatcher.so -I$(LDMDIR) -I$(DRPDIR) -L$(DRPDIR) -ldrprobe -lpthread -fPIC -shared -g -O3

clean:
	rm *.so
