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


#ifndef CONSOLE_H
#define CONSOLE_H

/* Normal and Dark/Light foreground */
#define X86_VIDEO_FG_BLACK     0
#define X86_VIDEO_FG_DKGRAY    8
#define X86_VIDEO_FG_BLUE      1
#define X86_VIDEO_FG_LTBLUE    9
#define X86_VIDEO_FG_GREEN     2
#define X86_VIDEO_FG_LTGREEN   10
#define X86_VIDEO_FG_CYAN      3
#define X86_VIDEO_FG_LTCYAN    11
#define X86_VIDEO_FG_RED       4
#define X86_VIDEO_FG_LTRED     12
#define X86_VIDEO_FG_MAGENTA   5
#define X86_VIDEO_FG_LTMAGENTA 13
#define X86_VIDEO_FG_BROWN     6
#define X86_VIDEO_FG_YELLOW    14
#define X86_VIDEO_FG_LTGRAY    7
#define X86_VIDEO_FG_WHITE     15
/* Background */
#define X86_VIDEO_BG_BLACK     (0 << 4)
#define X86_VIDEO_BG_BLUE      (1 << 4)
#define X86_VIDEO_BG_GREEN     (2 << 4)
#define X86_VIDEO_BG_CYAN      (3 << 4)
#define X86_VIDEO_BG_RED       (4 << 4)
#define X86_VIDEO_BG_MAGENTA   (5 << 4)
#define X86_VIDEO_BG_BROWN     (6 << 4)
#define X86_VIDEO_BG_LTGRAY    (7 << 4)


#define ORIG_X (0)
#define ORIG_Y (0)
#define ORIG_VIDEO_COLS (80)
#define ORIG_VIDEO_LINES (25)
#define VIDEO      0xB8000

void init_console(void);
void kputs(unsigned char *text);
void kputchar(unsigned char c);
void cls();


#endif /* console.h */
