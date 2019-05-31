#ifndef _DVTS_H_
#define _DVTS_H_

#define		SCT_HEADER		0x0
#define		SCT_SUBCODE		0x1
#define		SCT_VAUX		0x2
#define		SCT_AUDIO		0x3
#define		SCT_VIDEO		0x4

#define		DIFBLOCK_IN_IEEE1394_PKT	6
#define		IEEE1394_PKT_NUM	25

#define		DV_FORMAT_PAL	1
#define		DV_FORMAT_NTSC	2

#define	DIFSEQ_NUM_PAL		12
#define	F50_60_PARAM_PAL	0x80
#define	DIFSEQ_NUM_NTSC		10
#define	F50_60_PARAM_NTSC	0x00

struct dv_difblock {
  u_int32_t data[80/sizeof(u_int32_t)];
};

struct dv_difseq {
  struct dv_difblock header[1];
  struct dv_difblock subcode[2];
  struct dv_difblock vaux[3];

  struct dv_difblock audio[9];
  struct dv_difblock video[135];
};

struct dv_rawdata {
  struct dv_difseq dseq[DIFSEQ_NUM_PAL];
};

struct dv1394_pkts {
  u_int32_t pkt[DIFSEQ_NUM_PAL][IEEE1394_PKT_NUM][480/4];
};

struct dv_sct {
  u_int32_t	seq:4;
  u_int32_t	rsv0:1;
  u_int32_t	sct:3;

  u_int32_t	rsv1:4;
  u_int32_t	dseq:4;

  u_int32_t	dbn:8;
  u_int32_t	padding:8;
};


/* for reports */
#define		DVTS_REPORT_TYPE_DVSEND		0x00000001
#define		DVTS_REPORT_TYPE_DVRECV		0x00000002

struct dvsend_dest_info_obj {
  u_int16_t	frame_rate;
  u_int16_t	udp_port;

  u_int32_t	pkts_count;

  char		destStr[64];
};

struct dvsend_report_msg {
  u_int32_t	type;

  u_int32_t	flags;
  char		version[8];

  u_int32_t	n_dest_info_obj;
};

struct dvrecv_report_msg {
  u_int32_t	type;
  u_int32_t	flags;
  char		version[8];

  u_int32_t	pkts_count;
  u_int32_t	pkts_lost;
};
#endif	/* _DVTS_H_ */
