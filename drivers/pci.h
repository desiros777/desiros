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


#ifndef PCI_H
#define PCI_H




#define PCI_CONFIGURATION_ADDRESS 0x0CF8   // Address I/O Port
#define PCI_CONFIGURATION_DATA    0x0CFC   // Data    I/O Port
#define PMC	0x0CFB	// PCI Mechanism Configuration

#define PCI_VENDOR_ID   0x00 // length: 0x02 reg: 0x00 offset: 0x00
#define PCI_DEVICE_ID   0x02 // length: 0x02 reg: 0x00 offset: 0x02
#define PCI_COMMAND     0x04
#define PCI_STATUS      0x06
#define PCI_REVISION    0x08
#define PCI_CLASS       0x0B
#define PCI_SUBCLASS    0x0A
#define PCI_INTERFACE   0x09
#define PCI_HEADERTYPE  0x0
#define PCI_BAR0        0x10
#define PCI_BAR1        0x14
#define PCI_BAR2        0x18
#define PCI_BAR3        0x1C
#define PCI_BAR4        0x20
#define PCI_BAR5        0x24
#define PCI_CAPLIST     0x34
#define PCI_IRQLINE     0x3C

#define PCI_CMD_IO        BIT(0)
#define PCI_CMD_MMIO      BIT(1)
#define PCI_CMD_BUSMASTER BIT(2)

#define PCIBUSES      256
#define PCIDEVICES    32
#define PCIFUNCS      8


enum
{
    PCI_MMIO, PCI_IO, PCI_INVALIDBAR
};

typedef struct
{
    __u32 baseAddress;
    size_t   memorySize;
    __u8  memoryType;
} pciBar_t;

typedef struct
{
   __u16  vendorID;
   __u16  deviceID;
   __u8   classID;
   __u8   subclassID;
   __u8   interfaceID;
   __u8   revID;
   __u8   bus;
   __u8   device;
   __u8   func;
   __u8   irq;
   pciBar_t  bar[6];
   void*     data; // Pointer to internal data of associated driver.
} pciDev_t;





#endif
