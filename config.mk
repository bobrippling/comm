CC        = gcc
CFLAGS   += -g -Wextra -Wall -pedantic -pipe -std=c99

LD        = gcc
LDFLAGS  += -g

# make lacks ||, i think
ifeq ($(shell uname -o),MSys)
else
  ifeq ($(shell uname -o),Cygwin)
  else
    ifeq ($(shell uname -o),Msys)
    else
      CFLAGS   += -fstack-protector
      LDFLAGS  += -fstack-protector
    endif
  endif
endif

Q         = @

.c.o:
	@echo CC $<
	$Q${CC} -c ${CFLAGS} -o $@ $<

%.res: %.rc ../icons/%.ico
	@echo RC $^
	@windres -O coff -o $@ $<

mostlyclean:
	$Qrm -f *.o

# need to provide gladegen.h
# %gen.h
# %gen_pre.c %gen_post.c
%gen.c:
	@echo GEN $@
	@cat ../glade/gladegen_pre.c > $@
	@# awk script to paste comm.glade using multiple
	@# fputs() calls (strings have a limit in C)
	@sed 's/"/\\"/g' $^ | awk ' \
BEGIN { \
	ORS = ""; \
	n = 0; \
} \
{ \
	lines[n] = $$0; \
	n++; \
	if(n > 2){ \
		print "if(fputs(\n"; \
		for(i = 0; i < n; i++) \
			print "\"" lines[i] "\\n\"\n"; \
		n = 0; \
		print ", f) == EOF) goto bail;\n"; \
	} \
}' | sed 's/^\([",]\)/  \1/; s/^/  /' >> $@
	@cat ../glade/gladegen_post.c >> $@
