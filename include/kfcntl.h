/**
 * @file kfcntl.h
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3 of
 * the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details at
 * http://www.gnu.org/copyleft/gpl.html
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program; if not, see <http://www.gnu.org/licenses>.
 *
 */

#ifndef _KFCNTL_H
#define _KFCNTL_H



#define O_ACCMODE 00000003  /**< Access mode mask. */
#define O_RDONLY  00000000  /**< Open file for read only access. */
#define O_WRONLY  00000001  /**< Open file for write only access. */
#define O_RDWR    00000002  /**< Open file for both reading and writing. */
#ifndef O_CREAT
#define O_CREAT      00000100 /**< Create file if non-existant. */
#endif
#ifndef O_EXCL
#define O_EXCL    00000200 /* not fcntl */
#endif
#ifndef O_NOCTTY
#define O_NOCTTY  00000400 /* not fcntl */
#endif
#ifndef O_TRUNC
#define O_TRUNC      00001000 /*Truncate the file to zero length */
#endif
#ifndef O_APPEND
#define O_APPEND  00002000 /*Added at the end of file. */
#endif
#ifndef O_NONBLOCK
#define O_NONBLOCK   00004000 /*The read will not block*/
#endif
#ifndef O_SYNC
#define O_SYNC    00010000 /*Write call blocking until the data is not physically stored on the disk. */
#endif
#ifndef FASYNC
#define FASYNC    00020000 /* fcntl, for BSD compatibility */
#endif
#ifndef O_DIRECT
#define O_DIRECT  00040000 /**< direct disk access hint */
#endif
#ifndef O_DIRECTORY
#define O_DIRECTORY  00200000 /**< must be a directory */
#endif
#ifndef O_NOFOLLOW
#define O_NOFOLLOW   00400000 /**< don't follow links */
#endif
#ifndef O_NOATIME
#define O_NOATIME 01000000
#endif
#ifndef O_CLOEXEC
#define O_CLOEXEC 02000000 /**< set close_on_exec */
#endif

#define F_SETFL	4
#define F_GETFL	5


#endif
