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


#include <klibc.h>
#include <io.h>
#include <console.h>

/* These define our textpointer, our background and foreground
*  colors (video_attributes), and x and y cursor coordinates */
unsigned short *textmemptr;
int video_attrib = 0x0F;
int csr_x = 0, csr_y =0;
static unsigned video_att ;

/* Scrolls the screen */
void scroll(void)
{
    unsigned blank = 0 , temp;

    

    /* Row ORIG_VIDEO_LINES is the end, this means we need to scroll up */
    if(csr_y >= ORIG_VIDEO_LINES)
    {
        /* Move the current text chunk that makes up the screen
        *  back in the buffer by a line */
        temp = csr_y - ORIG_VIDEO_LINES + 1;
        memmove (textmemptr , textmemptr + temp * ORIG_VIDEO_COLS, (ORIG_VIDEO_LINES - temp) * ORIG_VIDEO_COLS * 2);
          /* Finally, we set the chunk of memory that occupies
        *  the last line of text to our 'blank' character */
        memset (textmemptr + (ORIG_VIDEO_LINES - temp) * ORIG_VIDEO_COLS, blank, 500);
       
        csr_y = ORIG_VIDEO_LINES - 1;
    }
}

/* Updates the hardware cursor: the little blinking line
*  on the screen under the last character pressed! */
void move_csr(void)
{
    unsigned temp;

    /* The equation for finding the index in a linear
    *  chunk of memory can be represented by:
    *  Index = [(y * width) + x] */
    temp = csr_y * ORIG_VIDEO_COLS + csr_x;

    /* This sends a command to indicies 14 and 15 in the
    *  CRT Control Register of the VGA controller. These
    *  are the high and low bytes of the index that show
    *  where the hardware cursor is to be 'blinking'. To
    *  learn more, you should look up some VGA specific
    *  programming documents. A great start to graphics:
    *  http://www.brackeen.com/home/vga */
    outb(0x3D4, 14);
    outb(0x3D5, temp >> 8);
    outb(0x3D4, 15);
    outb(0x3D5, temp);
}

/* Clears the screen */
void cls()
{

    int i;

   

    /* Sets the entire screen to spaces in our current
    *  color */
    for(i = 0; i < ORIG_VIDEO_LINES; i++)
        memset (textmemptr + i * ORIG_VIDEO_COLS, 0, ORIG_VIDEO_COLS);

    /* Update out virtual cursor, and then move the
    *  hardware cursor */
    csr_x = 0;
    csr_y = 0;
    move_csr();
}

/* Puts a single character on the screen */
void kputchar(unsigned char c)
{
    unsigned char *where;
    

    /* Handle a backspace, by moving the cursor back one space */
    if(c == 0x08)
    {
        if(csr_x != 0) csr_x--;
    }
    /* Handles a tab by incrementing the cursor's x, but only
    *  to a point that will make it divisible by 8 */
    else if(c == 0x09)
    {
        csr_x = (csr_x + 8) & ~(8 - 1);
    }
    /* Handles a 'Carriage Return', which simply brings the
    *  cursor back to the margin */
    else if(c == '\r')
    {
        csr_x = 0;
    }
    /* We handle our newlines the way DOS and the BIOS do: we
    *  treat it as if a 'CR' was also there, so we bring the
    *  cursor to the margin and we increment the 'y' value */
    else if(c == '\n')
    {
        csr_x = 0;
        csr_y++;
    }
    /* Any character greater than and including a space, is a
    *  printable character. The equation for finding the index
    *  in a linear chunk of memory can be represented by:
    *  Index = [(y * width) + x] */
    else if(c >= ' ')
    {
        where = textmemptr + ((csr_y * ORIG_VIDEO_COLS) + csr_x );
        *where = c ;  
       *(where + 1) = video_att;
        csr_x++;
    }

    /* If the cursor has reached the edge of the screen's width, we
    *  insert a new line in there */
    if(csr_x >= ORIG_VIDEO_COLS)
    {
        csr_x = 0;
        csr_y++;
    }

    /* Scroll the screen if needed, and finally move the cursor */
    scroll();
    move_csr();
}

/* Uses the above routine to output a string... */
void kputs(unsigned char *text)
{
    int i;

    for (i = 0; i < strlen((const char*)text); i++)
    {
        kputchar(text[i]);
    }
}



/* Sets our text-mode VGA pointer, then clears the screen for us */
void init_console(void)
{

  
    textmemptr = (unsigned short *)VIDEO;
    cls();
     video_att = X86_VIDEO_FG_WHITE  | X86_VIDEO_BG_BLUE ;
    kprintf("\tDESIROS kernel version : 0.0.1\n");
    textmemptr += 80 ;
   
   video_att = X86_VIDEO_FG_WHITE  | X86_VIDEO_BG_BLACK ;
}
