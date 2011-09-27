LDMDIR=/sampa/home/blucia/cvsandbox/Legerdemain

all: Scaley 

GLIB_CFLAGS=`pkg-config --cflags glib`
GLIB_LIBS=`pkg-config --libs glib`

Scaley:
	gcc Scaley.c -o Scaley.so -I$(LDMDIR) -L$(LDMDIR)/libs -lldm -fPIC -shared -g -O3

clean:
	rm *.so
