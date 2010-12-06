CC        = gcc
CFLAGS   += -g -Wextra -Wall -pedantic -pipe -std=c99

LD        = gcc
LDFLAGS  += -g

MAKEFLAGS = --no-print-directory
GTKINC     = gtk+-2.0 gmodule-2.0

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

%.res: %.rc
	@echo RC $^
	@windres -O coff -o $@ $<

mostlyclean:
	$Qrm -f *.o

%gen.h:
	@echo GEN $@
	@sed 's/"/\\"/g' $^ | awk ' \
		BEGIN { \
			ORS = ""; \
			n = 0; \
			\
			fname = ARGV[1]; \
			sub(/.*\//, "", fname); \
			sub(/\..*/, "", fname); \
			fnameU = toupper(fname); \
			\
			print "#ifndef _" fnameU "_H_\n"; \
			print "#define _" fnameU "_H_\n"; \
			print "const char *glade_str_" fname "[] = {\n"; \
			delete ARGV; \
		} \
		{ \
			print "\t\"" $$0 "\\n\", \n"; \
		} \
		END { \
			print "};\n#endif\n" \
		}' $< > $@
