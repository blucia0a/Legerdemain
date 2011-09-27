LDMDIR=/sampa/home/blucia/cvsandbox/Legerdemain

all: Scaley 

GLIB_CFLAGS=`pkg-config --cflags glib-2.0`
GLIB_LIBS=`pkg-config --libs glib-2.0`

Scaley:
	gcc Scaley.c -o Scaley.so -I$(LDMDIR) -L$(LDMDIR)/libs -lldm -fPIC -shared -g -O3 $(GLIB_CFLAGS) $(GLIB_LIBS)

clean:
	rm *.so
