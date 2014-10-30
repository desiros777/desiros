

#ifndef _KDIRENT_H
#define _KDIRENT_H

#include <types.h>
#include <kstat.h>

#define NAME_MAX 256 


struct dirent {
	__u32  d_ino; 
	__u16  d_reclen; 
	__u8   d_type; 
	char      d_name[NAME_MAX];
};


enum {
	DT_UNKNOWN = 0,
	DT_FIFO = 1,
	DT_CHR = 2,
	DT_DIR = 4,
	DT_BLK = 6,
	DT_REG = 8,
	DT_LNK = 10,
	DT_SOCK = 12,
	DT_WHT = 14
};


#endif
