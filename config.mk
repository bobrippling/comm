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
	$Qecho CC $<
	$Q${CC} -c ${CFLAGS} -o $@ $<

mostlyclean:
	$Qrm -f *.o

%gen.c: %gen_pre.c %gen_post.c
	@echo GEN $@
	@cat gladegen_pre.c > $@
	@# awk script to paste comm.glade using multiple
	@# fputs() calls (strings have a limit in C)
	@sed 's/"/\\"/g' glade/comm.glade glade/col.glade | awk ' \
BEGIN { \
	ORS = ""; \
	n = 0; \
} \
 \
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
	@cat gladegen_post.c >> $@
