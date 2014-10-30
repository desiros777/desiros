#include <kmalloc.h>
#include <block_dev.h>
#include <debug.h>
#include <kerrno.h>
#include <types.h>
#include <block_dev.h>

extern int blockdev_kernel_read(struct blockdev_instance * blockdev,
				   __u64 offset,
				   __u32 dest_buf,
				   __u32 * /* in/out */len);

/**
 * This structure defines the structure of a partition entry
 * (determined by the PC architecture)
 */
typedef struct partition_entry
{
  __u8  active;
  __u8  start_dl;
  __u16 start_cylinder;
  __u8  type;
  __u8  end_dl;
  __u16 end_cylinder;
  __u32 lba;
  __u32 size;
} partition_entry_t;

/**
 * Offset of the partition table inside the 512-byte sector.
 */
#define PART_TABLE_OFFSET 446

/**
 * The most common partition types. For a complete list, you can for
 * example refer to
 * http://www.win.tue.nl/~aeb/partitions/partition_types-1.html
 */
#define PART_TYPE_EXTENDED   0x5
#define PART_TYPE_FAT16_1    0xe
#define PART_TYPE_FAT16_2    0x6
#define PART_TYPE_FAT32_1    0xb
#define PART_TYPE_FAT32_2    0xc
#define PART_TYPE_LINUX_SWAP 0x82
#define PART_TYPE_LINUX      0x83

/**
 * Converts a partition type to a string
 */
static const char *
part_type_str (unsigned int type)
{
  switch (type)
    {
    case PART_TYPE_EXTENDED:
      return "Extended";
    case PART_TYPE_FAT16_1:
    case PART_TYPE_FAT16_2:
      return "FAT16";
    case PART_TYPE_FAT32_1:
    case PART_TYPE_FAT32_2:
      return "FAT32";
    case PART_TYPE_LINUX_SWAP:
      return "Linux Swap";
    case PART_TYPE_LINUX:
      return "Linux";
    default:
      return "Unknown";
    }
}

/**
 * Detect the partitions on the given device, and registers them to
 * the block device infrastructure.
 */
int part_detect (__u32 disk_class, __u32 disk_instance,
		 __u32 block_size, char *name)
{
  __u32 buffer;
  struct blockdev_instance *blkdev;
  struct partition_entry *part_entry;
  unsigned int extstart = 0, extsup = 0;
  int ret;
  __u32 size = block_size;
  unsigned int partnum;

          char * part_name;
          part_name =(char*) kmalloc(20, 0);
 

  blkdev = (struct blockdev_instance*)blockdev_ref_instance (disk_class, disk_instance);
  if (blkdev == NULL)
    return -ENOENT;

  buffer = (__u32) kmalloc (block_size, 0);
  if (buffer == 0)
    {
      blockdev_release_instance (blkdev);
      return -ENOMEM;
    }

 
 
  ret = blockdev_kernel_read (blkdev, 0, buffer, & size);

  if (ret != OK)
    {

      blockdev_release_instance (blkdev);
      kfree (buffer);
      return ret;
    }

if (size != block_size)
    {
      blockdev_release_instance (blkdev);
      kfree (buffer);
      return -EIO;
    }


  part_entry = (struct partition_entry *) (buffer + PART_TABLE_OFFSET);

 
  /* Handle primary partitions */
  for (partnum = 0; partnum < 4; partnum++)
    {

 
      if (part_entry [partnum].size == 0)
	continue;

      __u64 device_size  = (__u64) part_entry[partnum].size * block_size ;

      kprintf ("%s%d (%s, %lu Mb) \n", name, partnum+1,
			part_type_str(part_entry[partnum].type),
                        (device_size >> 20));

       snprintf (part_name, 20, "%s%d",name, partnum+1  );
   
      if (part_entry [partnum].type == PART_TYPE_EXTENDED)
	{
	  if (extstart != 0)
	    debug("Warning: two extended partitions detected\n");
	  extstart = part_entry [partnum].lba;
	  continue;
	}
      
      ret = blockdev_register_partition (part_name ,disk_class,
					     disk_instance + partnum + 1,
					     blkdev,
					     part_entry[partnum].lba,
					     part_entry[partnum].size, NULL);
      if (ret != OK)
	{
	  debug("Could not register partition %d for disk %lu:%lu: error %d\n",
			    partnum, disk_class, disk_instance, ret);
	  continue;
	}
    }

         
   
  while (extstart != 0 && partnum < 15)
    {
      ret = blockdev_kernel_read (blkdev, (extstart + extsup) * block_size,
				      (__u64) buffer, & size);
      if (ret != OK)
	{
          debug("error");
	  blockdev_release_instance (blkdev);
	  kfree (buffer);
	  return ret;
	}

    
     kprintf("%s%d (%lu Mb, %s)\n", name, partnum+1,
			(part_entry[0].size * block_size >> 20),
			part_type_str(part_entry[0].type));

      snprintf (part_name, sizeof(part_name), "%s%d",name, partnum+1  );
    
      ret = blockdev_register_partition (part_name ,disk_class,
					     disk_instance + partnum + 1,
					     blkdev,
					     extstart + part_entry[0].lba,
					     part_entry[0].size, NULL);
      if (ret != OK)
	debug ("Could not register partition %d for disk %lu:%lu: error %d\n",
			  partnum, disk_class, disk_instance, ret);

      extsup = part_entry[1].lba;
      partnum ++;
      if (extsup == 0)
	break;
    }

  blockdev_release_instance (blkdev);
  kfree (buffer);
  return OK;
}


