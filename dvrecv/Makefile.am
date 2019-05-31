bin_PROGRAMS = dvrecv

dvrecv_MANS = dvrecv.1

dvrecv_SOURCES = \
	flags.h			\
	ieee1394-struct.h	\
	ieee1394.h		\
	info.h			\
	multicast.h		\
	param.h			\
	read.h			\
	rtcp.h			\
	shm.h			\
	sig-behavior.h		\
	udp.h			\
	write.h			\
				\
	main.c		\
	info.c		\
	multicast.c	\
	read.c		\
	shm.c		\
	sig-behavior.c	\
	udp.c		\
	rtcp.c

dvrecv_DEPENDENCIES = \
	@DVRECV_LDADD@

dvrecv_LDADD = \
	@DVRECV_LDADD@

dvrecv_LDFLAGS = \
	@DVRECV_LDFLAGS@

INCLUDES = \
	@DVRECV_INCLUDES@
