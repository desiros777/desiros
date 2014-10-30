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
#include <console.h>
#include <kmalloc.h>

void *memset (void * s, int c, int n) {
    unsigned char* d = (unsigned char*) s;

    while (n--)
        *d++ = (char)c;
   return s;
}

void *memmove(void *to, const void *from, int length){
           
  char *dstp;
  const char *srcp;

  srcp = from;
  dstp = to;
  if (srcp < dstp)
    while (length-- != 0)
      dstp[length] = srcp[length];
  else
    while (length-- != 0)
      *dstp++ = *srcp++;

  return to;

}

void *memcpy(void *dst0, const void *src0, register unsigned int size)
{
  char *dst;
  const char *src;
  for (dst = (char*)dst0, src = (const char*)src0 ;
       size > 0 ;
       dst++, src++, size--)
    *dst = *src;
  return dst0;
}

char *strcpy(char *dest, const char *src)
{
    char *ret;

    ret = dest;
    while (*src) {
        *dest++ = *src++;
    }
    *dest = '\0';
    return ret;
}

char *strncpy(char * s1, const char * s2, size_t n)
{
	size_t i;
	for(i=0 ; s2[i] != '\0' && i<n ; i++)
	{
		s1[i] = s2[i];
	}
	for(; i<n ; i++)
	{
		s1[i] = '\0';
	}
	
	return s1;
}

char *strzcpy(register char *dst, register const char *src, register int len)
{
  int i;

  if (len <= 0)
    return dst;
  
  for (i = 0; i < len; i++)
    {
      dst[i] = src[i];
      if(src[i] == '\0')
        return dst;
    }
  
  dst[len-1] = '\0'; 
  return dst;
}

int strcmp(const char *dst,const char *src)
{
	int i = 0;

	while ((dst[i] == src[i])) {
		if (src[i++] == 0)
			return 0;
	}

	return 1;
}

size_t strlen(const char* s)
{
	register const char *p = s;
	while (*p) p++;
	return p - s;
}

char *strdup (const char *s)
{
	int len = strlen(s);
	char* new_string = (char*) kmalloc((len+1)*sizeof(char),0); // len + 1 pour le '\0'
	
	return strcpy(new_string, s);
}

char *strchrnul(const char *s, int c) {
	char *i;
	for (i = (char*)s; *i != '\0'; ++i) {
		if (*i == c) {
			return i;
		}
	}
	return i;
}


void itoa (char *buf, int base, int d)
     {
       char *p = buf;
       char *p1, *p2;
       unsigned long ud = d;
       int divisor = 10;
     
       /* If %d is specified and D is minus, put `-' in the head. */
       if (base == 'd' && d < 0)
         {
           *p++ = '-';
           buf++;
           ud = -d;
         }
       else if (base == 'x')
         divisor = 16;
     
       /* Divide UD by DIVISOR until UD == 0. */
       do
         {
           int remainder = ud % divisor;
     
           *p++ = (remainder < 10) ? remainder + '0' : remainder + 'a' - 10;
         }
       while (ud /= divisor);
     
       /* Terminate BUF. */
       *p = 0;
     
       /* Reverse BUF. */
       p1 = buf;
       p2 = p - 1;
       while (p1 < p2)
         {
           char tmp = *p1;
           *p1 = *p2;
           *p2 = tmp;
           p1++;
           p2--;
         }
     }
     

typedef unsigned char *va_list;

/* width of stack == width of int */
#define	STACKITEM	int

/* round up width of objects pushed on stack. The expression before the
& ensures that we get 0 for objects of size 0. */
#define	VA_SIZE(TYPE)					\
	((sizeof(TYPE) + sizeof(STACKITEM) - 1)	\
		& ~(sizeof(STACKITEM) - 1))

/* &(LASTARG) points to the LEFTMOST argument of the function call
(before the ...) */
#define	va_start(AP, LASTARG)	\
	(AP=((va_list)&(LASTARG) + VA_SIZE(LASTARG)))

#define va_end(AP)	/* nothing */

#define va_arg(AP, TYPE)	\
	(AP += VA_SIZE(TYPE), *((TYPE *)(AP - VA_SIZE(TYPE))))



typedef void (*fn_ptr_t)(unsigned char c);
static int do_printf(const char *fmt, va_list args, fn_ptr_t fn );

void kprintf(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	(void)do_printf(fmt, args, kputchar);
	va_end(args);
}

/* flags used in processing format string */
#define		PR_LJ	0x01	/* left justify */
#define		PR_CA	0x02	/* use A-F instead of a-f for hex */
#define		PR_SG	0x04	/* signed numeric conversion (%d vs. %u) */
#define		PR_32	0x08	/* long (32-bit) numeric conversion */
#define		PR_16	0x10	/* short (16-bit) numeric conversion */
#define		PR_WS	0x20	/* PR_SG set and num was < 0 */
#define		PR_LZ	0x40	/* pad left with '0' instead of ' ' */
#define		PR_FP	0x80	/* pointers are far */

/* largest number handled is 2^32-1, lowest radix handled is 8.
2^32-1 in base 8 has 11 digits (add 5 for trailing NUL and for slop) */
#define		PR_BUFLEN	16

static int do_printf(const char *fmt, va_list args, fn_ptr_t fn )
{
	unsigned state, flags, radix, actual_wd, count, given_wd;
	unsigned char *where, buf[PR_BUFLEN];
	long num;

	state = flags = count = given_wd = 0;
/* begin scanning format specifier list */
	for(; *fmt; fmt++)
	{
		switch(state)
		{
/* STATE 0: AWAITING % */
		case 0:
			if(*fmt != '%')	/* not %... */
			{
				fn(*fmt);	/* ...just echo it */
				count++;
                            if(*fmt == '\n')
                               count = 0;
				break;
			}
                      
/* found %, get next char and advance state to check if next char is a flag */
			state++;
			fmt++;
			/* FALL THROUGH */
/* STATE 1: AWAITING FLAGS (%-0) */
		case 1:
			if(*fmt == '%')	/* %% */
			{
				fn(*fmt);
				count++;
				state = flags = given_wd = 0;
				break;
			}
			if(*fmt == '-')
			{
				if(flags & PR_LJ)/* %-- is illegal */
					state = flags = given_wd = 0;
				else
					flags |= PR_LJ;
				break;
			}
/* not a flag char: advance state to check if it's field width */
			state++;
/* check now for '%0...' */
			if(*fmt == '0')
			{
				flags |= PR_LZ;
				fmt++;
			}
			/* FALL THROUGH */
/* STATE 2: AWAITING (NUMERIC) FIELD WIDTH */
		case 2:
			if(*fmt >= '0' && *fmt <= '9')
			{
				given_wd = 10 * given_wd +
					(*fmt - '0');
				break;
			}
/* not field width: advance state to check if it's a modifier */
			state++;
			/* FALL THROUGH */
/* STATE 3: AWAITING MODIFIER CHARS (FNlh) */
		case 3:
			if(*fmt == 'F')
			{
				flags |= PR_FP;
				break;
			}
			if(*fmt == 'N')
				break;
			if(*fmt == 'l')
			{
				flags |= PR_32;
				break;
			}
			if(*fmt == 'h')
			{
				flags |= PR_16;
				break;
			}
/* not modifier: advance state to check if it's a conversion char */
			state++;
			/* FALL THROUGH */
/* STATE 4: AWAITING CONVERSION CHARS (Xxpndiuocs) */
		case 4:
			where = buf + PR_BUFLEN - 1;
			*where = '\0';
			switch(*fmt)
			{
			case 'X':
				flags |= PR_CA;
				/* FALL THROUGH */
/* xxx - far pointers (%Fp, %Fn) not yet supported */
			case 'x':
			case 'p':
			case 'n':
				radix = 16;
				goto DO_NUM;
			case 'd':
			case 'i':
				flags |= PR_SG;
				/* FALL THROUGH */
			case 'u':
				radix = 10;
				goto DO_NUM;
			case 'o':
				radix = 8;
/* load the value to be printed. l=long=32 bits: */
DO_NUM:				if(flags & PR_32)
					num = va_arg(args, unsigned long);
/* h=short=16 bits (signed or unsigned) */
				else if(flags & PR_16)
				{
					if(flags & PR_SG)
						num = va_arg(args, short);
					else
						num = va_arg(args, unsigned short);
				}
/* no h nor l: sizeof(int) bits (signed or unsigned) */
				else
				{
					if(flags & PR_SG)
						num = va_arg(args, int);
					else
						num = va_arg(args, unsigned int);
				}
/* take care of sign */
				if(flags & PR_SG)
				{
					if(num < 0)
					{
						flags |= PR_WS;
						num = -num;
					}
				}
/* convert binary to octal/decimal/hex ASCII
OK, I found my mistake. The math here is _always_ unsigned */
				do
				{
					unsigned long temp;

					temp = (unsigned long)num % radix;
					where--;
					if(temp < 10)
						*where = temp + '0';
					else if(flags & PR_CA)
						*where = temp - 10 + 'A';
					else
						*where = temp - 10 + 'a';
					num = (unsigned long)num / radix;
				}
				while(num != 0);
				goto EMIT;
			case 'c':
/* disallow pad-left-with-zeroes for %c */
				flags &= ~PR_LZ;
				where--;
				*where = (unsigned char)va_arg(args,
					unsigned char);
				actual_wd = 1;
				goto EMIT2;
			case 's':
/* disallow pad-left-with-zeroes for %s */
				flags &= ~PR_LZ;
				where = va_arg(args, unsigned char *);
EMIT:
				actual_wd = strlen((const char *)where);
				if(flags & PR_WS)
					actual_wd++;
/* if we pad left with ZEROES, do the sign now */
				if((flags & (PR_WS | PR_LZ)) ==
					(PR_WS | PR_LZ))
				{
					fn('-');
					count++;
				}
/* pad on left with spaces or zeroes (for right justify) */
EMIT2:				if((flags & PR_LJ) == 0)
				{
					while(given_wd > actual_wd)
					{
						fn(flags & PR_LZ ? '0' :' ');
						count++;
						given_wd--;
					}
				}
/* if we pad left with SPACES, do the sign now */
				if((flags & (PR_WS | PR_LZ)) == PR_WS)
				{
					fn('-');
					count++;
				}
/* emit string/char/converted number */
				while(*where != '\0')
				{
					fn(*where++);
					count++;
				}
/* pad on right with spaces (for left justify) */
				if(given_wd < actual_wd)
					given_wd = 0;
				else given_wd -= actual_wd;
				for(; given_wd; given_wd--)
				{
					fn(' ');
					count++;
				}
				break;
			default:
				break;
			}
		default:
			state = flags = given_wd = 0;
			break;
		}
	}
	return count;
 }



int vsnprintf(char *buff, __u32 len, const char * format, va_list ap)
{
  __u32 i, result;
  bool fmt_modifiers = false;
  bool prefix_long = false;
  bool prefix_long_long = false;
  
  if (!buff || !format || (len <= 0))
    return -1;
  
#define PUTCHAR(thechar) \
  do { \
    if (result < len-1) \
      *buff++ = (thechar);  \
    result++; \
  } while (0)
  
  result = 0;
  for(i=0 ; format[i] != '\0' ; i++)
    {
      if (!fmt_modifiers && (format[i] != '%'))
	{
	  PUTCHAR(format[i]);
	  continue;
	}
	
      switch (format[i])
	{
	case '%':
	  if (fmt_modifiers)
	    {
	      PUTCHAR('%');
	      fmt_modifiers = false;
	      break;
	    }
	  
	  fmt_modifiers    = true;
	  prefix_long      = false;
	  prefix_long_long = false;
	  break;
	  
	case 'l':
	  if (prefix_long)
	    prefix_long_long = true;
	  else
	    prefix_long = true;
	  break;
	  
	case 'u':
	  {
	    if (! prefix_long_long)
	      {
		unsigned int integer = va_arg(ap,unsigned int);
		int cpt2 = 0;
		char buff_int[16];
		
		do {
		  int m10 = integer%10;
		  buff_int[cpt2++]=(char)('0'+ m10);
		  integer=integer/10;
		} while(integer!=0);
	    
		for(cpt2 = cpt2 - 1 ; cpt2 >= 0 ; cpt2--)
		  PUTCHAR(buff_int[cpt2]);
	      }
	    else
	      {
		unsigned long long int integer
		  = va_arg(ap,unsigned long long int);
		int cpt2 = 0;
		char buff_int[32];
		
		do {
		  int m10 = integer%10;
		  buff_int[cpt2++]=(char)('0'+ m10);
		  integer=integer/10;
		} while(integer!=0);
	    
		for(cpt2 = cpt2 - 1 ; cpt2 >= 0 ; cpt2--)
		  PUTCHAR(buff_int[cpt2]);
	      }
	  }
	  fmt_modifiers = false;
	  break;

	case 'i':
	case 'd':
	  {
	    if (! prefix_long_long)
	      {
		int integer = va_arg(ap,int);
		int cpt2 = 0;
		char buff_int[16];
		
		if (integer<0)
		  PUTCHAR('-');
		/* Ne fait pas integer = -integer ici parce que INT_MIN
		   n'a pas d'equivalent positif (int = [-2^31, 2^31-1]) */
		
		do {
		  int m10 = integer%10;
		  m10 = (m10 < 0)? -m10:m10;
		  buff_int[cpt2++]=(char)('0'+ m10);
		  integer=integer/10;
		} while(integer!=0);
	    
		for(cpt2 = cpt2 - 1 ; cpt2 >= 0 ; cpt2--)
		  PUTCHAR(buff_int[cpt2]);
	      }
	    else
	      {
		long long int integer = va_arg(ap,long long int);
		int cpt2 = 0;
		char buff_int[32];
		
		if (integer<0)
		  PUTCHAR('-');
		/* Ne fait pas integer = -integer ici parce que INT_MIN
		   n'a pas d'equivalent positif (int = [-2^63, 2^63-1]) */
		
		do {
		  int m10 = integer%10;
		  m10 = (m10 < 0)? -m10:m10;
		  buff_int[cpt2++]=(char)('0'+ m10);
		  integer=integer/10;
		} while(integer!=0);
	    
		for(cpt2 = cpt2 - 1 ; cpt2 >= 0 ; cpt2--)
		  PUTCHAR(buff_int[cpt2]);
	      }
	  }
	  fmt_modifiers = false;
	  break;
	  
	case 'c':
	  {
	    int value = va_arg(ap,int);
	    PUTCHAR((char)value);
	    fmt_modifiers = false;
	    break;
	  }
	  
	case 's':
	  {
	    char const*string = va_arg(ap,char const*);
	    if (! string)
	      string = "(null)";
	    for( ; *string != '\0' ; string++)
	      PUTCHAR(*string);
	    fmt_modifiers = false;
	    break;
	  }
	  
	case 'p':
	  PUTCHAR('0');
	  PUTCHAR('x');
	case 'x':
	  {
	    unsigned long long int hexa;
	    unsigned long long int nb;
	    int x, had_nonzero = 0;
	    
	    if (prefix_long_long)
	      hexa = va_arg(ap,unsigned long long int);
	    else
	      hexa = va_arg(ap,unsigned int);
	    
	    for(x=0 ; x < 16 ; x++)
	      {
		nb = (unsigned long long int)(hexa << (x*4));
		nb = (nb >> 60) & 0xf;
		// Skip the leading zeros
		if (nb == 0)
		  {
		    if (had_nonzero)
		      PUTCHAR('0');
		  }
		else
		  {
		    had_nonzero = 1;
		    if (nb < 10)
		      PUTCHAR('0'+nb);
		    else
		      PUTCHAR('a'+(nb-10));
		  }
	      }
	    if (! had_nonzero)
	      PUTCHAR('0');
	  }
	  fmt_modifiers = false;
	  break;
	  
	default:
	  PUTCHAR('%');
	  if (prefix_long)
	    PUTCHAR('l');
	  if (prefix_long_long)
	    PUTCHAR('l');
	  PUTCHAR(format[i]);
	  fmt_modifiers = false;
	}
    }

  *buff = '\0';
  return result;
}


int snprintf(char * buff, __u32 len, const char *format, ...)
{
  va_list ap;
 
  va_start(ap, format);
  len = vsnprintf(buff, len, format, ap);
  va_end(ap);
 
  return len;
}
