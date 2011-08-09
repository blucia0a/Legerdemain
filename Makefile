all:  a2l a2ltest applier scaley ldm
applier:
	gcc Applier.c -o libapplier.so -fPIC -shared -g -O3 -ldl
ldm:
	gcc legerdemain.c -o libldm.so -fPIC -shared -g -O3 -ldl -lreadline -L. -laddr2line -lapplier
scaley:
	gcc scaley_LDM.c -o scaley_LDM.so -L. -lapplier -fPIC -shared -g -O3 -ldl
test:
	gcc scaleytest.c -o scaleytest -g -L. -lldm
a2l:
	gcc -g -shared -fPIC -lbfd addr2line.c -o libaddr2line.so
a2ltest:
	gcc -g ./testLibA2L.c -o testLibA2L -L. -laddr2line -liberty 
clean:
	rm *.so 
