library := ticrypt.so
submit := job_submit_ticrypt.so
doc := doc/ticrypt-spank.8.gz
dev := fake.txt 
slurm := slurm/config.h
ifndef LIB
LIB := /usr/lib64/slurm
endif
ifndef INC
INC := /usr/include/slurm
endif
ifndef SLURM_VERSION
SLURM_VERSION=slurm-19-05-4-1
endif


.PHONY: all
all: $(library) $(doc) $(submit)

$(library):
	gcc -g -Wall -I$(INC) -Iinclude/ -fPIC -shared -o $(library) src/spank/ticrypt.c -lconfig

$(doc):
	cp doc/ticrypt-spank doc/ticrypt-spank.8
	gzip doc/ticrypt-spank.8

$(submit):	$(slurm)
	gcc -DHAVE_CONFIG_H -Islurm/ -I/usr/include/slurm/ -g -O2 -pthread -fno-gcse -fPIC -Werror -Wall -g -O0 -fno-strict-aliasing -MT job_submit_ticrypt.lo -MD -MP -MF src/ticrypt_submit/.deps/job_submit_ticrypt.Tpo -c src/ticrypt_submit/job_submit_ticrypt.c -o src/ticrypt_submit/.libs/job_submit_ticrypt.o -lconfig
	mv -f src/ticrypt_submit/.deps/job_submit_ticrypt.Tpo src/ticrypt_submit/.deps/job_sumit_ticrypt.Plo
	gcc -shared -fPIC -DPIC src/ticrypt_submit/.libs/job_submit_ticrypt.o -O2 -pthread -O0 -pthread -Wl,-soname -Wl,$(submit) -o $(submit) -lconfig

$(dev):	$(slurm)
	gcc -g -Wall -I$(INC) -Iinclude/ -fPIC -shared -o $(LIB)/$(library)  src/spank/ticrypt.c -lconfig
	gcc -DHAVE_CONFIG_H -I$(INC) -Islurm/ -g -O2 -pthread -fno-gcse -fPIC -Werror -Wall -g -O0 -fno-strict-aliasing -MT job_submit_ticrypt.lo -MD -MP -MF src/ticrypt_submit/.deps/job_submit_ticrypt.Tpo -c src/ticrypt_submit/job_submit_ticrypt.c -o src/ticrypt_submit/.libs/job_submit_ticrypt.o -lconfig
	mv -f src/ticrypt_submit/.deps/job_submit_ticrypt.Tpo src/ticrypt_submit/.deps/job_sumit_ticrypt.Plo
	gcc -I$(INC) -shared -fPIC -DPIC src/ticrypt_submit/.libs/job_submit_ticrypt.o -O2 -pthread -O0 -pthread -Wl,-soname -Wl,$(submit) -o $(LIB)/$(submit) -lconfig
	
$(slurm):
	git clone https://github.com/SchedMD/slurm.git ./slurm;cd slurm;git checkout $(SLURM_VERSION);./configure;

.PHONY: library
library: $(library)

.PHONY: submit
submit: $(submit)

.PHONY: doc
doc: $(doc)

.PHONY: dev
dev: $(dev)

.PHONY: slurm
slurm: $(slurm)

install: $(library) $(doc) $(submit)
	mkdir -p $(LIB)
	install -m 0755 $(library) $(LIB)/$(library)
	install -m 0640 config/ticrypt-spank.conf /etc/ticrypt-spank.conf
	install -m 0644 $(doc) /usr/local/share/man/man8/ticrypt-spank.8.gz
	install -m 0755 $(plugin) $(LIB)/$(plugin)

clean:
	rm -f $(library)
	rm -f $(submit)
	rm -f *.o
	rm -f doc/ticrypt-spank.8* 
	rm -f fake.txt
	rm -rf src/ticrypt_submit/.libs/*
	rm -rf slurm/


