/*
   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
   USA.
*/

#include <types.h>
#include <kmalloc.h>
#include <io.h>
#include <kerrno.h>
#include <debug.h>
#include <klibc.h>
#include <block_dev.h>
#include <partition.h>
#include <ksynch.h>

/**
 * @file ide.c
 *
 * Basic PIO IDE implementation based on the ATA standards
 * http://www.t13.org/
 */

/** IDE major */
#define BLOCKDEV_IDE_MAJOR            1


/**
 * Busy-wait for a given time
 *
 * @param delay Delay to wait
 *
 * @todo Implement a calibration routine
 */
static void udelay(int delay)
{
  int i;

  for (i = 0; i < (delay * 1000) ; i++)
    {
      i++; i--;
    }
}

/*
 * Each IDE controller is controlled through a set of 9 I/O ports,
 * starting from a base address, which is different for each IDE
 * controller. On a standard PC with two onboard IDE controllers, the
 * I/O registers of the first controller start at 0x1F0
 * (IDE_CONTROLLER_0_BASE), the I/O registers of the second controller
 * start at 0x170 (IDE_CONTROLLER_1_BASE). These I/O registers are
 * 8-bits wide, and can be read using the inb() macro, and written
 * using the outb() macro.
 *
 * The first controller is generally known as "primary" controller,
 * and the second one is know as "secondary" controller. Each of them
 * can handle at most two devices : the "master" device and the
 * "slave" device.
 *
 * The registers can be used to issue commands and send data to the
 * IDE controller, for example to identify a device, to read or write
 * a device. Then are different ways of transmitting data to the IDE
 * controller :
 *
 * - program the IDE controller, send the data, and wait for the
 *   completion of the request. This method is called "polling".
 *
 * - program the IDE controller, send the data, block the current
 *   process and do something else. The completion of the request will
 *   be signaled asynchronously by an interrupt (IRQ), which will
 *   allow to restart the sleeping process.
 *
 * - program the IDE controller and the DMA controller. There's no
 *   need to transfer the data to the IDE controller : the DMA
 *   controller will do it. This allows to use the CPU to do something
 *   useful during the transfer of data between the main memory and
 *   the IDE controller.
 *
 * In this driver, we use the two first methods. The polling method is
 * used to fetch the identification of the devices, while the IRQ
 * method is used to read and write to devices.
 */

#define IDE_CONTROLLER_0_BASE           0x1F0
#define IDE_CONTROLLER_1_BASE           0x170

#define IDE_CONTROLLER_0_IRQ            14
#define IDE_CONTROLLER_1_IRQ            15

/*
 * All defines below are relative to the base address of the I/O
 * registers (IDE_CONTROLLER_0_BASE and IDE_CONTROLLER_1_BASE)
 */

/**
 * Read/write register that allows to transfer the data to be written
 * or to fetch the read data from the IDE controller.
 */
#define ATA_DATA                        0x00

/**
 * Read only register that gives information about errors that occured
 * during operation
 */
#define ATA_ERROR                       0x01

/**
 * Write only register to set precompensation. It is in fact the same
 * register as the ATA_ERROR register. It simply has a different
 * behaviour when reading and writing it.
 */
#define ATA_PRECOMP                     0x01

/**
 * Write only register used to set the count of sectors on which the
 * request applies.
 */
#define ATA_SECTOR_COUNT                0x02

/**
 * Write only register used to set the number of the starting sector
 * of the request.
 */
#define ATA_SECTOR_NUMBER               0x03

/**
 * Write only register used to set the 8 lower bits of the starting
 * cylinder number of the request
 */
#define ATA_CYL_LSB                     0x04

/**
 * Write only register used to set the 8 higher bits of the starting
 * cylinder number of the request
 */
#define ATA_CYL_MSB                     0x05

/**
 * Write only register that allows to select whether the LBA mode
 * should be used, and to select whether the request concerns the
 * slave or master device.
 */
#define ATA_DRIVE                       0x06
#define         ATA_D_IBM               0xa0    
#define         ATA_D_SET               0xe0    /* bits that must be set */
#define         ATA_D_LBA               0x40    /* use LBA ? */
#define         ATA_D_MASTER            0x00    /* select master */
#define         ATA_D_SLAVE             0x10    /* select slave */

/**
 * Read only register that contains the status of the controller. Each
 * bit of this register as a different signification.
 */
#define ATA_STATUS                      0x07
#define         ATA_S_ERROR             0x01    /* error */
#define         ATA_S_INDEX             0x02    /* index */
#define         ATA_S_CORR              0x04    /* data corrected */
#define         ATA_S_DRQ               0x08    /* data request */
#define         ATA_S_DSC               0x10    /* drive Seek Completed */
#define         ATA_S_DWF               0x20    /* drive write fault */
#define         ATA_S_DRDY              0x40    /* drive ready */
#define         ATA_S_BSY               0x80    /* busy */

/**
 * Write only register used to set the command that the IDE controller
 * should process. In our driver, only ATA_C_ATA_IDENTIFY, ATA_C_READ
 * and ATA_C_WRITE are used.
 */
#define ATA_CMD                         0x07
#define         ATA_C_ATA_IDENTIFY      0xec    /* get ATA params */
#define         ATA_C_ATAPI_IDENTIFY    0xa1    /* get ATAPI params*/
#define         ATA_C_READ              0x20    /* read command */
#define         ATA_C_WRITE             0x30    /* write command */
#define         ATA_C_READ_MULTI        0xc4    /* read multi command */
#define         ATA_C_WRITE_MULTI       0xc5    /* write multi command */
#define         ATA_C_SET_MULTI         0xc6    /* set multi size command */
#define         ATA_C_PACKET_CMD        0xa0    /* set multi size command */

/**
 * Read register that contains more information about the status of
 * the controller
 */
#define ATA_ALTPORT                     0x206   /* (R) alternate
                                                   Status register */

/**
 * Write only register that allows to control the controller
 */
#define ATA_DEVICE_CONTROL              0x206   /* (W) device control
                                                   register */
#define         ATA_A_nIEN              0x02    /* disable interrupts */
#define         ATA_A_RESET             0x04    /* RESET controller */
#define         ATA_A_4BIT              0x08    /* 4 head bits */

/** Magic numbers used to detect ATAPI devices */
#define ATAPI_MAGIC_LSB                 0x14
#define ATAPI_MAGIC_MSB                 0xeb



#define MAX_IDE_CONTROLLERS 2
#define MAX_IDE_DEVICES     2

#define IDE_BLK_SIZE        512

#define IDE_DEVICE(ctrl,device) (((ctrl) * 2) + (device))
#define IDE_MINOR(ctrl,device)  (IDE_DEVICE(ctrl,device)*16)

typedef enum { IDE_DEVICE_NONE,
	       IDE_DEVICE_HARDDISK,
	       IDE_DEVICE_CDROM } ide_device_type_t;

struct ide_controller;
struct ide_device {
  int id;
  ide_device_type_t type;
  enum { IDE_DEVICE_MASTER, IDE_DEVICE_SLAVE } position;
  int cyls;
  int heads;
  int sectors;
  __u64 blocks;
  bool support_lba;
  struct ide_controller *ctrl;
};

/* This structure describe the informations returned by the Identify
   Device command (imposed by the ATA standard) */
struct ide_device_info
{
  __u16 general_config_info;      /* 0 */
  __u16 nb_logical_cylinders;     /* 1 */
  __u16 reserved1;                /* 2 */
  __u16 nb_logical_heads;         /* 3 */
  __u16 unformatted_bytes_track;  /* 4 */
  __u16 unformatted_bytes_sector; /* 5 */
  __u16 nb_logical_sectors;       /* 6 */
  __u16 vendor1[3];               /* 7-9 */
  __u8  serial_number[20];        /* 10-19 */
  __u16 buffer_type;              /* 20 */
  __u16 buffer_size;              /* 21 */
  __u16 ecc_bytes;                /* 22 */
  __u8  firmware_revision[8];     /* 23-26 */
  __u8  model_number[40];         /* 27-46 */
  __u8  max_multisect;            /* 47 */
  __u8  vendor2;
  __u16 dword_io;                 /* 48 */
  __u8  vendor3;                  /* 49 */
  __u8  capabilities;
  __u16 reserved2;                /* 50 */
  __u8  vendor4;                  /* 51 */
  __u8  pio_trans_mode;
  __u8  vendor5;                  /* 52 */
  __u8  dma_trans_mode;
  __u16 fields_valid;             /* 53 */
  __u16 cur_logical_cylinders;    /* 54 */
  __u16 cur_logical_heads;        /* 55 */
  __u16 cur_logical_sectors;      /* 56 */
  __u16 capacity1;                /* 57 */
  __u16 capacity0;                /* 58 */
  __u8  multsect;                 /* 59 */
  __u8  multsect_valid;
  __u32 lba_capacity;             /* 60-61 */
  __u16 dma_1word;                /* 62 */
  __u16 dma_multiword;            /* 63 */
  __u16 pio_modes;                /* 64 */
  __u16 min_mword_dma;            /* 65 */
  __u16 recommended_mword_dma;    /* 66 */
  __u16 min_pio_cycle_time;       /* 67 */
  __u16 min_pio_cycle_time_iordy; /* 68 */
  __u16 reserved3[11];            /* 69-79 */
  __u16 major_version;            /* 80 */
  __u16 minor_version;            /* 81 */
  __u16 command_sets1;            /* 82 */
  __u16 command_sets2;            /* 83 dixit lk : b14 (smart enabled) */
  __u16 reserved4[4];             /* 84-87 */
  __u16 dma_ultra;                /* 88 dixit lk */
  __u16 reserved5[37];            /* 89-125 */
  __u16 last_lun;                 /* 126 */
  __u16 reserved6;                /* 127 */
  __u16 security;                 /* 128 */
  __u16 reserved7[127];
} __attribute__((packed));
struct ide_controller {
  int id;
  int ioaddr;
  int irq;
  enum { IDE_CTRL_NOT_PRESENT, IDE_CTRL_PRESENT } state;
  struct ide_device devices[MAX_IDE_DEVICES];
  struct kmutex mutex;
};

struct ide_controller ide_controllers[MAX_IDE_CONTROLLERS] = {
  {
    .id     = 0,
    .ioaddr = IDE_CONTROLLER_0_BASE,
    .irq    = IDE_CONTROLLER_0_IRQ,
    .state  = IDE_CTRL_NOT_PRESENT,
  },
  {
    .id     = 1,
    .ioaddr = IDE_CONTROLLER_1_BASE,
    .irq    = IDE_CONTROLLER_1_IRQ,
    .state  = IDE_CTRL_NOT_PRESENT
  }
};



static int ide_get_device_info (struct ide_device *dev)
{
  int devselect;
  __u16 info[256];
  int timeout, i;
  int status;

  if(dev->type != IDE_DEVICE_HARDDISK) debug() ;

  if (dev->position == IDE_DEVICE_MASTER)
    devselect = ATA_D_MASTER;
  else
    devselect = ATA_D_SLAVE;

  /* Ask the controller to NOT send interrupts to acknowledge
     commands */
  outb(dev->ctrl->ioaddr + ATA_DEVICE_CONTROL, ATA_A_nIEN | ATA_A_4BIT );
  udelay(1);

  /* Select the device (master or slave) */
  outb( dev->ctrl->ioaddr + ATA_DRIVE, ATA_D_SET | devselect );

  /* Send the IDENTIFY command */
  outb(dev->ctrl->ioaddr + ATA_CMD, ATA_C_ATA_IDENTIFY );

  /* Wait for command completion (wait while busy bit is set) */
  for(timeout = 0; timeout < 30000; timeout++)
    {
      status = inb(dev->ctrl->ioaddr + ATA_STATUS);
      if(!(status & ATA_S_BSY))
        break;

      udelay(1);
    }

  /* DRQ bit indicates that data is ready to be read. If it is not set
     after an IDENTIFY command, there is a problem */
  if(! (status & ATA_S_DRQ))
    {
      return ENXIO;
    }

     
  /* Read data from the controller buffer to a temporary buffer */
  for(i = 0; i < 256; i++)
    info[i] = inw(dev->ctrl->ioaddr + ATA_DATA);
 


  /* Fetch intersting informations from the structure */
  dev->heads   = (unsigned int) info[3];
  dev->cyls    = (unsigned int) info[1];
  dev->sectors = (unsigned int) info[6];

    dev->support_lba = 0;
    dev->support_lba = (info[49] >> 9) & 1;

/* Determines the capacity of the device */
  if (dev->heads   == 16 &&
      dev->sectors == 63 &&
      dev->cyls    == 16383)
    {
      if (dev->support_lba)
        dev->blocks = info[60];
      else
        return -ENXIO;
    }
  else
    dev->blocks = dev->cyls * dev->sectors * dev->heads ;


  return OK;
}


static ide_device_type_t ide_probe_device (struct ide_device *dev)
{
  int status;
  int devselect;

  if (dev->position == IDE_DEVICE_MASTER)
    devselect = ATA_D_MASTER;
  else
    devselect = ATA_D_SLAVE;

  /* Select the given device */
  outb (dev->ctrl->ioaddr + ATA_DRIVE, ATA_D_SET | devselect );

  /* Read the status of the device */
  status = inb(dev->ctrl->ioaddr + ATA_STATUS);

  /* If status indicates a busy device, it means that there's no
     device */
  if (status & ATA_S_BSY)
    return IDE_DEVICE_NONE;

  /* If status indicates that drive is ready and drive has complete
     seeking, then we've got an hard drive */
  if (status & (ATA_S_DRDY | ATA_S_DSC))
    return IDE_DEVICE_HARDDISK;

  /* Detect CD-ROM drives by reading the cylinder low byte and
     cylinder high byte, and check if they match magic values */
  if(inb(dev->ctrl->ioaddr + ATA_CYL_LSB) == ATAPI_MAGIC_LSB &&
     inb(dev->ctrl->ioaddr + ATA_CYL_MSB) == ATAPI_MAGIC_MSB)
    return IDE_DEVICE_CDROM;

  return IDE_DEVICE_NONE;
}

static int ide_probe_controller(struct ide_controller *ctrl)
{
 
    kmutex_init (& ctrl->mutex, "ide-mutex");

  /* Master */
  ctrl->devices[0].id       = 0;
  ctrl->devices[0].position = IDE_DEVICE_MASTER;
  ctrl->devices[0].ctrl     = ctrl;
  ctrl->devices[0].type     = ide_probe_device (& ctrl->devices[0]);

 if (ctrl->devices[0].type == IDE_DEVICE_HARDDISK){
      ide_get_device_info (& ctrl->devices[0]);
       }
 
    

  /* Slave */
  ctrl->devices[1].id       = 1;
  ctrl->devices[1].position = IDE_DEVICE_SLAVE;
  ctrl->devices[1].ctrl     = ctrl;
  ctrl->devices[1].type     = ide_probe_device (& ctrl->devices[1]);

   if (ctrl->devices[1].type == IDE_DEVICE_HARDDISK){
      ide_get_device_info (& ctrl->devices[1]);
    }
  

  return OK;
}


static int ide_io_operation (struct ide_device *dev,
				   void* buf, __u64 block,
				   bool iswrite)
{

 
  __u8 cyl_lo, cyl_hi, sect, head, status;
  int devselect, i;
 


  if (dev->position == IDE_DEVICE_MASTER){
    devselect = ATA_D_MASTER;
  }else{
    devselect = ATA_D_SLAVE;
   }

 
   if (dev->support_lba)
    {
      sect   = (block & 0xff);
      cyl_lo = (block >> 8) & 0xff;
      cyl_hi = (block >> 16) & 0xff;
      head   = ((block >> 24) & 0x7) | 0x40;
    }
  else
    {
      int cylinder = block /
	(dev->heads * dev->sectors);
      int temp = block %
	(dev->heads * dev->sectors);
      cyl_lo = cylinder & 0xff;
      cyl_hi = (cylinder >> 8) & 0xff;
      head   = temp / dev->sectors;
      sect   = (temp % dev->sectors) + 1;
    }
    
/* Make sure nobody is using the same controller at the same time */
   kmutex_lock (& dev->ctrl->mutex, NULL);
      
/* Select device */
  outb(dev->ctrl->ioaddr + ATA_DRIVE,ATA_D_SET  | devselect );
  udelay(100);

  /* Write to registers */
  outb(dev->ctrl->ioaddr + ATA_SECTOR_COUNT,1);
  outb(dev->ctrl->ioaddr + ATA_SECTOR_NUMBER, sect );
  outb(dev->ctrl->ioaddr + ATA_CYL_LSB, cyl_lo);
  outb(dev->ctrl->ioaddr + ATA_CYL_MSB, cyl_hi );
 if (dev->support_lba){
       outb(dev->ctrl->ioaddr + ATA_DRIVE, ATA_D_SET | devselect  << 4 | head ); 
   }else
       outb(dev->ctrl->ioaddr + ATA_DRIVE, ATA_D_IBM | devselect  << 4 | head ); // Drive & CHS.



  /* Send the command, either read or write */
  if (iswrite){
    outb(dev->ctrl->ioaddr + ATA_CMD, ATA_C_WRITE);
  }else
    outb(dev->ctrl->ioaddr + ATA_CMD, ATA_C_READ );
      
 /* Wait for the device ready to transfer */
   while (inb(dev->ctrl->ioaddr + ATA_STATUS) & ATA_S_BSY);



  /* If an error was detected, stop here */
  if (inb(dev->ctrl->ioaddr + ATA_STATUS) & ATA_S_ERROR)
    {
      return -ENXIO;
    }



  /* If it's a write I/O, transfer the contents of the buffer provided
     by the user to the controller internal buffer, so that the
     controller can write the data to the disk */
  if (iswrite)
    {
      /* Wait for the DRQ bit to be set */
      while (1)
	{
	  status = inb(dev->ctrl->ioaddr + ATA_STATUS);
	  if (status & ATA_S_ERROR)
	    {
              kmutex_unlock (& dev->ctrl->mutex);
	      return -ENXIO;
	    }
	    
	  if (!(status & ATA_S_BSY) && (status & ATA_S_DRQ))
	    break;
	}


      /* Do the transfer to the device's buffer */
      __u16 *buffer = (__u16 *) buf;
      for (i = 0 ; i < 256 ; i++)
	outw ( dev->ctrl->ioaddr + ATA_DATA, buffer[i]);

      /* Wait for the device to have received all the data */
      while (1)
	{
	  status = inb(dev->ctrl->ioaddr + ATA_STATUS);
	  if (status & ATA_S_ERROR)
	    {
              kmutex_unlock (& dev->ctrl->mutex);
	      return -ENXIO;
	    }
	    
	  if (!(status & ATA_S_BSY) && !(status & ATA_S_DRQ))
	    break;
	}
    }



  /* ATA specs tell to read the alternate status reg and ignore its
     result */
  //inb(dev->ctrl->ioaddr + ATA_ALTPORT);

  if (! iswrite)
    {

      /* Wait for the DRQ bit to be set */
      while (1)
	{
	  status = inb(dev->ctrl->ioaddr + ATA_STATUS);
	  if (status & ATA_S_ERROR)
	    {
              kmutex_unlock (& dev->ctrl->mutex);
	      return -ENXIO;
	    }
	    
	  if (!(status & ATA_S_BSY) && (status & ATA_S_DRQ))
	    break;
	}


      /* copy data from the controller internal buffer to the buffer
	 provided by the user */
      __u16 *buffer = (__u16 *) buf;
      for (i = 0 ; i < 256 ; i++)
	*buffer++ = inw (dev->ctrl->ioaddr + ATA_DATA);

      /* ATA specs tell to read the alternate status reg and ignore its
	 result */
      inb(dev->ctrl->ioaddr + ATA_ALTPORT);
    }

/* If an error was detected, stop here */
  if (inb(dev->ctrl->ioaddr + ATA_STATUS) & ATA_S_ERROR)
    {
      return -1;
    }

    kmutex_unlock (& dev->ctrl->mutex);
  return OK;
}

int
ide_read_device (void *blkdev_instance, void* dest_buf,
		 __u64 block_offset)
{

  struct ide_device *dev;
  dev = (struct ide_device *) blkdev_instance;

  return ide_io_operation (dev, dest_buf, block_offset, false);
}

int ide_write_device (void *blkdev_instance, void* src_buf,
		  __u64 block_offset)
{
  struct ide_device *dev;

  dev = (struct ide_device *) blkdev_instance;

  return ide_io_operation (dev, src_buf, block_offset, true);
}


static struct blockdev_operations ide_ops = {
  .read_block  = ide_read_device,
  .write_block = ide_write_device,
  .ioctl       = NULL
};


static int ide_register_devices (void)
{
  int ctrl, dev;
  int ret;

  for (ctrl = 0; ctrl < MAX_IDE_CONTROLLERS ; ctrl++)
    {
      for (dev = 0; dev < MAX_IDE_DEVICES ; dev++)
	{
	  
          char * name;
          name =(char*) kmalloc(20, 0);

	  struct ide_device *device;

	  device = &ide_controllers[ctrl].devices[dev];
           

	  snprintf (name, sizeof(name), "hd%c", ('a' + IDE_DEVICE(ctrl, dev)));

	  if (device->type == IDE_DEVICE_HARDDISK)
	    {

	      kprintf("%s: harddisk %lu Mb \n", name,
			       (device->blocks * IDE_BLK_SIZE >> 20));
	      ret = blockdev_register_disk (name ,BLOCKDEV_IDE_MAJOR,
						IDE_MINOR(ctrl, dev),
						IDE_BLK_SIZE, device->blocks,
						128, &ide_ops, device);
	      if (ret != OK)
		{
		  kprintf("Warning: could not register disk %s \n", name );
		  continue;
		}
            
	      ret = part_detect (BLOCKDEV_IDE_MAJOR,
				     IDE_MINOR(ctrl, dev), IDE_BLK_SIZE,
				     name);
	      if (ret != OK)
		{
		  debug ("Warning could not detect partitions (%d)>\n",
				    ret);
		  continue;
		}
               
	
	    }

	  else if (device->type == IDE_DEVICE_CDROM)
	    debug("%s: CDROM\n", name);
	}
    }
              
  return OK;
}

int ide_subsystem_setup (void)
{
  int ret;

  ret = ide_probe_controller(& ide_controllers[0]);
  if (ret != OK)
    debug("Error while probing IDE controller 0\n");

  ret =ide_probe_controller(& ide_controllers[1]);
  if (ret != OK)
    debug("Error while probing IDE controller 1\n");

  ide_register_devices ();

  return OK;
}



