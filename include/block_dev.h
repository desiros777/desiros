
#ifndef _BLKDEV_H_
#define _BLKDEV_H_

struct blockdev_operations {

  /** @note MANDATORY */
  int (*read_block)(void * blockdev_instance_custom_data,
			  void* dest_buf /* Kernel address */,
			  __u64 block_offset);


  int (*write_block)(void * blockdev_instance_custom_data,
			   void* src_buf /* Kernel address */,
			   __u64 block_offset);


  /**
   * @note Also called when an ioctl is made to a partition
   */
  int (*ioctl)(void * blockdev_instance_custom_data,
		     int req_id,
		     __u32 req_arg );
};


int blockdev_register_disk (const char* name,__u32     device_class,
			    __u32     device_instance,
			    int     block_size,
			    __u64   number_of_blocks,
			    __u32    blkcache_size_in_blocks,
			    struct blockdev_operations * blockdev_ops,
			    void * blockdev_instance_custom_data);



#endif


