DRPDIR=/sampa/home/blucia/cvsandbox/DRProbe/drprobe/
LDMDIR=/sampa/home/blucia/cvsandbox/Legerdemain

all: ConcurrentWatcher 

GLIB_CFLAGS=`pkg-config --cflags glib`
GLIB_LIBS=`pkg-config --libs glib`

ConcurrentWatcher:
	
	gcc X86Decoding.c ConcurrentWatcher.c -o ConcurrentWatcher.so -I$(LDMDIR) -L$(LDMDIR)/libs -lldm -I$(DRPDIR) -L$(DRPDIR) -ldrprobe -lpthread -fPIC -shared -g -O3 -ldisasm $(GLIB_CFLAGS) $(GLIB_LIBS)

clean:
	rm *.so
