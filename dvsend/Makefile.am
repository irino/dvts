bin_PROGRAMS = dvsend

dvsend_MANS = dvsend.1

dvsend_SOURCES = \
	flags.h		\
	ieee1394.h	\
	info.h		\
	multicast.h	\
	param.h		\
	rtcp.h		\
	rtp.h		\
	sig-behavior.h	\
	udp.h		\
			\
	main.c		\
	info.c		\
	multicast.c	\
	rtp.c		\
	rtcp.c		\
	sig-behavior.c	\
	udp.c

dvsend_DEPENDENCIES = \
	@DVSEND_LDADD@

dvsend_LDADD = \
	@DVSEND_LDADD@

dvsend_LDFLAGS = \
	@DVSEND_LDFLAGS@

INCLUDES = \
	@DVSEND_INCLUDES@
