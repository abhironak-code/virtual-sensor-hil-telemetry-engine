
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>
#include <linux/ktime.h>
#include <linux/string.h>
#include "vsensor_ioctl.h"

#define AMBIENT_MC     25000
#define HEAT_PER_PCT   10        
static int noise_mc = 150;
module_param(noise_mc, int, 0444);
MODULE_PARM_DESC(noise_mc, "Peak sensor noise in milli-degC (default 150)");

struct vs_dev {
	struct mutex lock;
	s32 temp_mc;      /* true plant temperature   */
	u32 heater;       /* 0..100 %                 */
	u32 fault;        /* enum vs_fault_mode       */
	u32 seq;
	u32 lcg;          /* noise generator state    */
	s32 stuck_val;
	bool stuck_latched;
	struct vs_stats st;
};

static struct vs_dev vs;

static void vs_reset_locked(void)
{
	vs.temp_mc = AMBIENT_MC;
	vs.heater = 0;
	vs.fault = VS_FAULT_NONE;
	vs.seq = 0;
	vs.lcg = 12345;
	vs.stuck_latched = false;
	memset(&vs.st, 0, sizeof(vs.st));
}

static s32 vs_noise(s32 amp)
{
	if (amp <= 0)
		return 0;
	vs.lcg = vs.lcg * 1664525u + 1013904223u;       
	return (s32)((vs.lcg >> 16) % (2 * amp + 1)) - amp;
}

static int vs_open(struct inode *inode, struct file *filp)
{
	return 0;
}

static ssize_t vs_read(struct file *filp, char __user *buf, size_t len,
		       loff_t *off)
{
	struct vs_sample s;
	u32 p, tri;
	s32 temp, hum;

	if (len < sizeof(s))
		return -EINVAL;

	mutex_lock(&vs.lock);

	
	vs.temp_mc += (s32)vs.heater * HEAT_PER_PCT -
		      (vs.temp_mc - AMBIENT_MC) / 100;
	vs.seq++;
	vs.st.reads++;

	if (vs.fault == VS_FAULT_DROPOUT) {
		vs.st.dropouts++;
		mutex_unlock(&vs.lock);
		return -EIO;
	}

	
	temp = vs.temp_mc + vs_noise(noise_mc);
	s.flags = 0;

	if (vs.fault == VS_FAULT_STUCK) {
		if (!vs.stuck_latched) {
			vs.stuck_val = temp;
			vs.stuck_latched = true;
		}
		temp = vs.stuck_val;
		s.flags |= VS_FLAG_FAULT;
		vs.st.faults_served++;
	} else {
		vs.stuck_latched = false;
		if (vs.fault == VS_FAULT_SPIKE && (vs.seq % 10) == 0) {
			temp += 20000;
			s.flags |= VS_FLAG_FAULT;
			vs.st.faults_served++;
		}
	}

	
	hum = 50000 - (vs.temp_mc - AMBIENT_MC) / 2;
	hum = clamp_t(s32, hum, 0, 100000);
	p = vs.seq % 200;
	tri = p < 100 ? p : 200 - p;

	s.ts_ns      = ktime_get_ns();
	s.seq        = vs.seq;
	s.temp_mc    = temp;
	s.hum_mpct   = hum;
	s.press_pa   = 101325 + ((s32)tri - 50) * 3;
	s.heater_pct = vs.heater;

	mutex_unlock(&vs.lock);

	if (copy_to_user(buf, &s, sizeof(s)))
		return -EFAULT;
	return sizeof(s);
}

static long vs_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	u32 v;
	long ret = 0;

	switch (cmd) {
	case VS_IOC_SET_HEATER:
		if (get_user(v, (u32 __user *)arg))
			return -EFAULT;
		if (v > 100)
			return -EINVAL;
		mutex_lock(&vs.lock);
		vs.heater = v;
		mutex_unlock(&vs.lock);
		break;
	case VS_IOC_SET_FAULT:
		if (get_user(v, (u32 __user *)arg))
			return -EFAULT;
		if (v > VS_FAULT_DROPOUT)
			return -EINVAL;
		mutex_lock(&vs.lock);
		vs.fault = v;
		vs.stuck_latched = false;
		mutex_unlock(&vs.lock);
		break;
	case VS_IOC_RESET:
		mutex_lock(&vs.lock);
		vs_reset_locked();
		mutex_unlock(&vs.lock);
		break;
	case VS_IOC_GET_STATS:
		mutex_lock(&vs.lock);
		if (copy_to_user((void __user *)arg, &vs.st, sizeof(vs.st)))
			ret = -EFAULT;
		mutex_unlock(&vs.lock);
		break;
	default:
		return -ENOTTY;
	}
	return ret;
}

static const struct file_operations vs_fops = {
	.owner          = THIS_MODULE,
	.open           = vs_open,
	.read           = vs_read,
	.unlocked_ioctl = vs_ioctl,
};

static struct miscdevice vs_misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name  = VS_DEV_NAME,
	.fops  = &vs_fops,
	.mode  = 0666,           
};

static int __init vs_init(void)
{
	int ret;

	mutex_init(&vs.lock);
	vs_reset_locked();
	ret = misc_register(&vs_misc);
	if (ret) {
		pr_err("vsensor: misc_register failed (%d)\n", ret);
		return ret;
	}
	pr_info("vsensor: /dev/%s registered (noise=%d mC)\n",
		VS_DEV_NAME, noise_mc);
	return 0;
}

static void __exit vs_exit(void)
{
	misc_deregister(&vs_misc);
	pr_info("vsensor: unregistered\n");
}

module_init(vs_init);
module_exit(vs_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Virtual sensor / thermal-plant device for HIL telemetry engine");
