#include <linux/modules.h>

#define DEV_MEM_SIZE 512

/*Device memory*/
char device_buffer[DEV_MEM_SIZE];

static int __init pcd_driver_init(void)
{
	
}

static void __exit pcd_driver_cleanup(void)
{
	
}

module_init(pcd_driver_init);
module_exit(pcd_driver_cleanup);