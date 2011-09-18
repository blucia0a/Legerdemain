DRPDIR=/sampa/home/blucia/cvsandbox/DRProbe/drprobe/

all: ConcurrentWatcher 

ConcurrentWatcher:
	gcc ConcurrentWatcher.c -o ConcurrentWatcher.so -I$(DRPDIR) -L$(DRPDIR) -ldrprobe -lpthread -fPIC -shared -g -O3

clean:
	rm *.so
