
#ifndef _DEV_ZERO_H_
#define _DEV_ZERO_H_


//  "Driver" to map /dev/zero in user space



#include <uvmm.h>
#include <types.h>

int dev_zero_subsystem_setup();


// Map /dev/zero into user space

int dev_zero_map(struct uvmm_as * dest_as,
			   __u32 *uaddr,
			   __u32 size,
			   __u32 access_rights,
			   __u32 flags);

#endif 
