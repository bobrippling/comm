GTKINC   = gtk+-2.0 gmodule-2.0
CFLAGS  += `pkg-config --cflags ${GTKINC}`
LDFLAGS += `pkg-config --libs   ${GTKINC}`

GUI_DEPS = comm.o gtkutil.o gladegen.o ../libcomm/libcomm.a

guicomm: ${GUI_DEPS}
	$Qecho LD $@
	$Q${CC} ${LDFLAGS} -o $@ $^

gladegen.c: gladegen_pre.c comm.glade gladegen_post.c
	@echo GEN $@
	@cat gladegen_pre.c > $@
	@# awk script to paste comm.glade using multiple
	@# fputs() calls (strings have a limit in C)
	@sed 's/"/\\"/g' comm.glade | awk ' \
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
}' | sed 's/^\([",]\)/\t\1/; s/^/\t/' >> $@
	@cat gladegen_post.c >> $@

clean: mostlyclean
	$Qrm -f comm gladegen.c

wincomm: ${GUI_DEPS} ../common/sockprintf.o
	${CC} ${CFLAGS} -o guicomm $^ ${LDFLAGS} -lws2_32

include ../config.mk
