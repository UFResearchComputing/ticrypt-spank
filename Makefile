library := ticrypt.so
doc := doc/ticrypt-spank.8.gz
dev := fake.txt 
ifndef DESTDIR
DESTDIR := /opt/slurm
endif


.PHONY: all
all: $(library) $(doc)

$(library):
	gcc -g -Wall -I$(DESTDIR)/include/ -Iinclude/ -fPIC -shared -o $(library) src/ticrypt.c -lconfig

$(doc):
	cp doc/ticrypt-spank doc/ticrypt-spank.8
	gzip doc/ticrypt-spank.8

$(dev):
	gcc -g -Wall -I$(DESTDIR)/include/ -Iinclude/ -fPIC -shared -o $(DESTDIR)/lib64/slurm/ticrypt.so  src/ticrypt.c -lconfig
	

.PHONY: library
library: $(library)

.PHONY: doc
doc: $(doc)

.PHONY: dev
dev: $(dev)

install: $(library) $(doc)
	mkdir -p $(DESTDIR)/lib64/slurm
	install -m 0755 $(library) $(DESTDIR)/lib64/slurm/ticrypt.so	
	install -m 0640 config/ticrypt-spank.conf /etc/ticrypt-spank.conf
	install -m 0644 $(doc) /usr/local/share/man/man8/ticrypt-spank.8.gz

clean:
	rm -f $(library)
	rm -f *.o
	rm -f doc/ticrypt-spank.8* 
	rm -f fake.txt
