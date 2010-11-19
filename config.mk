CC        = gcc
CFLAGS   += -g -Wextra -Wall -pedantic -pipe -std=c99

LD        = gcc
LDFLAGS  += -g

MAKEFLAGS = --no-print-directory

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

%gen.h:
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
			print "#ifndef " fnameU "_H\n"; \
			print "#define " fnameU "_H\n"; \
			print "const char *glade_str_" fname "[] = {\n"; \
			delete ARGV; \
		} \
		{ \
			print "\t\"" $$0 "\\n\", \n"; \
		} \
		END { \
			print "};\n#endif\n" \
		}' $< >> $@
