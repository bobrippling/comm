.PHONY : clean mostlyclean all \
	server libcomm term glade

all: server libcomm term glade

term:  libcomm
glade: libcomm

server:
	$Qmake -C $@

libcomm:
	$Qmake -C $@

term:
	$Qmake -C $@

glade:
	$Qmake -C $@

clean:
	$Qrm -f server/server glade/comm term/comm
	$Qfind . -iname \*.o|xargs rm -f

include config.mk
