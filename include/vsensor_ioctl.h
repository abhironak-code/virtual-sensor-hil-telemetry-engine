
#ifndef VSENSOR_IOCTL_H
#define VSENSOR_IOCTL_H

#include <linux/types.h>
#include <linux/ioctl.h>

#define VS_DEV_NAME   "vsensor0"          
#define VS_DEV_PATH   "/dev/" VS_DEV_NAME
#define VS_MAGIC      'v'


struct vs_sample {
	__u64 ts_ns;        /* CLOCK_MONOTONIC timestamp                */
	__u32 seq;          /* sample counter since reset               */
	__s32 temp_mc;      /* temperature, milli-degree Celsius        */
	__s32 hum_mpct;     /* relative humidity, milli-percent         */
	__s32 press_pa;     /* pressure, Pascal                         */
	__u32 heater_pct;   /* actuator value currently applied (0-100) */
	__u32 flags;        /* VS_FLAG_*                                */
};

#define VS_FLAG_FAULT   0x1u              


enum vs_fault_mode {
	VS_FAULT_NONE    = 0,
	VS_FAULT_STUCK   = 1,   /* output freezes at last value      */
	VS_FAULT_SPIKE   = 2,   /* +20 C glitch every 10th sample    */
	VS_FAULT_DROPOUT = 3,   /* read() fails with -EIO            */
};

struct vs_stats {
	__u64 reads;
	__u32 faults_served;
	__u32 dropouts;
};

#define VS_IOC_SET_HEATER  _IOW(VS_MAGIC, 1, __u32)
#define VS_IOC_SET_FAULT   _IOW(VS_MAGIC, 2, __u32)
#define VS_IOC_RESET       _IO (VS_MAGIC, 3)
#define VS_IOC_GET_STATS   _IOR(VS_MAGIC, 4, struct vs_stats)

#endif 
