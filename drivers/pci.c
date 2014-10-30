/* Copyright (C) 2004,2005  The DESIROS Team
    desiros.dev@gmail.com

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

#include <io.h>
#include <klibc.h>
#include "pci.h"
#include <kmalloc.h>

//! A structure to write to the PCI configuration register.
//! It represents a location on the PCI bus.
typedef struct confadd
{
    __u8 reg:8;
    __u8 func:3;
    __u8 dev:5;
    __u8 bus:8;
    __u8 rsvd:7;
    __u8 enable:1;
} confadd_t;

__u32 pci_config_read(__u8 bus, __u8 dev, __u8 func, __u8 reg, __u32 length)
{
          __u16 base;
 
        union {
                confadd_t c;
                __u32 n;
        } u;
         u.n = 0;
         u.c.enable = 1;
         u.c.rsvd = 0;
         u.c.bus = bus;
         u.c.dev = dev;
         u.c.func = func;
         u.c.reg = reg & 0xFC;
 
         outl(PCI_CONFIGURATION_ADDRESS, u.n);
         base = PCI_CONFIGURATION_DATA + (reg & 0x03);
 
         switch(length)
         {
                 case 1: return inb(base);
                 case 2: return inw(base);
                 case 4: return inl(base);
                 default: return 0;
         }
}


void pci_config_write(__u8 bus, __u8 dev, __u8 func, __u8 reg, __u32 val, __u32 length)
{
     __u16 base;
 
         union {
                confadd_t c;
                 __u32 n;
         } u;

        u.n = 0;
        u.c.enable = 1;
        u.c.rsvd = 0;
        u.c.bus = bus;
        u.c.dev = dev;
        u.c.func = func;
        u.c.reg = reg & 0xFC;
        base = PCI_CONFIGURATION_DATA + (reg & 0x03);
       outl(PCI_CONFIGURATION_ADDRESS, u.n);

       switch(length)
        {
                case 1: outb(base, (__u8) val); break;
                case 2: outw(base, (__u16) val); break;
                 case 4: outl(base, val); break;
         }
}


void pci_scan(void)
{

       cli;
 
    kprintf("   => PCI devices:");

    kprintf("\nB:D:F\tIRQ\tDescription");
    kprintf("\n--------------------------------------------------------------------------------");
  

    __u64 counter = 0;
__u16 bus;
    for ( bus = 0; bus < PCIBUSES; ++bus)
    {
              __u8 device;
        for (device = 0; device < PCIDEVICES; ++device)
        {
            __u8 headerType = pci_config_read(bus, device, 0, PCI_HEADERTYPE, 1);
            __u8 funcCount = PCIFUNCS;
            if (!(headerType & 0x80)) // Bit 7 in header type (Bit 23-16) - multifunctional
            {
                funcCount = 1; //  not multifunctional, only function 0 used
            }
              __u8 func;
            for ( func = 0; func < funcCount; ++func)
            {
                __u16 vendorID = pci_config_read(bus, device, func, PCI_VENDOR_ID, 2);
                if (vendorID && vendorID != 0xFFFF)
                {
                    pciDev_t *PCIdev = (pciDev_t *) kmalloc(sizeof(pciDev_t),0);
                

                    PCIdev->data        = 0;
                    PCIdev->vendorID    = vendorID;
                    PCIdev->deviceID    = pci_config_read(bus, device, func, PCI_DEVICE_ID, 2);
                    PCIdev->classID     = pci_config_read(bus, device, func, PCI_CLASS, 1);
                    PCIdev->subclassID  = pci_config_read(bus, device, func, PCI_SUBCLASS, 1);
                    PCIdev->interfaceID = pci_config_read(bus, device, func, PCI_INTERFACE, 1);
                    PCIdev->revID       = pci_config_read(bus, device, func, PCI_REVISION, 1);
                    PCIdev->irq         = pci_config_read(bus, device, func, PCI_IRQLINE, 1);
                  
                

                    PCIdev->bus    = bus;
                    PCIdev->device = device;
                    PCIdev->func   = func;

                    // Screen output
                    if (PCIdev->irq != 255)
                    {
                              

                    
                        kprintf("%d:%d.%d\t%d", PCIdev->bus, PCIdev->device, PCIdev->func, PCIdev->irq);
                        kprintf("\tvend: %xh, dev: %xh", PCIdev->vendorID, PCIdev->deviceID);
                  

                          // Install device driver
                        if (PCIdev->classID == 0x0C && PCIdev->subclassID == 0x03) // USB Host Controller
                        {
                            kprintf("\t USB Host Controller");
                        }

                        if (PCIdev->classID == 0x02 && PCIdev->subclassID == 0x00) // Network Adapters
                        {
                            kprintf("\t Network Adapters");
                        }

                        if (PCIdev->classID == 0x04 && PCIdev->subclassID == 0x01) // Multimedia Controller Audio
                        {
                            kprintf("\t Multimedia Controller Audio");
                        }

                       
                   
                        kputchar('\n');
                 kfree((__u32) PCIdev);

                        counter++;
                    } // if irq != 255
                } // if pciVendor
            } // for function
        } // for device
    } // for bus
    
    kprintf("--------------------------------------------------------------------------------\n");
     sti;
}


