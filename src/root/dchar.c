/* Digital Mars DMDScript source code.
 * Copyright (c) 2000-2001 by Chromium Communications
 * Copyright (c) 2004-2007 by Digital Mars
 * All Rights Reserved, written by Walter Bright
 * http://www.digitalmars.com/dscript/cppscript.html
 *
 * This software is for evaluation purposes only.
 * It may not be redistributed in binary or source form,
 * incorporated into any product or program,
 * or used for any purpose other than evaluating it.
 * To purchase a license,
 * see http://www.digitalmars.com/dscript/cpplicense.html
 *
 * Use at your own risk. There is no warranty, express or implied.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "printf.h"
#include "dchar.h"
#include "mem.h"

#if defined(UNICODE)

#ifdef linux

void wchar_case(wchar_t c, wchar_t xcase[2])
{
    xcase[0] = c;
    xcase[1] = c;

    if (c >= 'A' && c <= 'Z')
    {
        xcase[1] = c + 32;
    }
    else if (c >= 'a' && c <= 'z')
    {
        xcase[0] = c - 32;
    }
    if (c < 0x80)		// if ASCII, most common case
	return;

    if ((c >= 0x00C0 && c <= 0x00D6) || (c >= 0x00D8 && c<=0x00DE))
    {
        xcase[1] = c + 32;
    }
    else if ((c >= 0x00E0 && c <= 0x00F6) || (c >= 0x00F8 && c <= 0x00FE))
    {
        xcase[0] = c - 32;
    }
    else if (c == 0x00FF)
    {
        xcase[0] = 0x0178;
    }
    else if ((c >= 0x0100 && c < 0x0138) || (c > 0x0149 && c < 0x0178))
    {
        if (c == 0x0130)
            xcase[1] = 0x0069;
        else if (c == 0x0131)
            xcase[0] = 0x0049;
	else if ((c & 1) == 0)
            xcase[1] = c + 1;
	else
            xcase[0] = c - 1;
    }
    else if (c == 0x0178)
    {
        xcase[1] = 0x00FF;
    }
    else if ((c >= 0x0139 && c < 0x0149) || (c > 0x0178 && c < 0x017F))
    {
        if (c & 1) {
            xcase[1] = c+1;
        } else {
            xcase[0] = c-1;
        }
    }
    else if (c == 0x017F)
    {
        xcase[0] = 0x0053;
    }
    else if (c >= 0x0200 && c <= 0x0217)
    {
        if ((c & 1) == 0) {
            xcase[1] = c+1;
        } else {
            xcase[0] = c-1;
        }
    }
    else if ((c >= 0x0401 && c <= 0x040C) || (c>= 0x040E && c <= 0x040F))
    {
        xcase[1] = c + 80;
    }
    else if (c >= 0x0410  && c <= 0x042F)
    {
        xcase[1] = c + 32;
    }
    else if (c >= 0x0430 && c<= 0x044F)
    {
        xcase[0] = c - 32;
    }
    else if ((c >= 0x0451 && c <= 0x045C) || (c >=0x045E && c<= 0x045F))
    {
        xcase[0] = c - 80;
    }
    else if (c >= 0x0460 && c <= 0x047F)
    {
        if ((c & 1) == 0)
            xcase[1] = c + 1;
        else
            xcase[0] = c - 1;
    }
    else if (c >= 0x0531 && c <= 0x0556)
    {
        xcase[1] = c + 48;
    }
    else if (c >= 0x0561 && c < 0x0587)
    {
        xcase[0] = c - 48;
    }
    else if (c >= 0x10A0 && c <= 0x10C5)
    {
        xcase[1] = c + 48;
    }
    else if (c >= 0x10D0 && c <= 0x10F5)
    {
    }
    else if (c >= 0xFF21 && c <= 0xFF3A)
    {
        xcase[1] = c + 32;
    }
    else if (c >= 0xFF41 && c <= 0xFF5A)
    {
        xcase[0] = c - 32;
    }
}

int Dchar::isLower(dchar c)
{
    wchar_t xcase[2];

    wchar_case(c, xcase);
    return iswalpha(c) && xcase[1] == c;
}

int Dchar::isUpper(dchar c)
{
    wchar_t xcase[2];

    wchar_case(c, xcase);
    return iswalpha(c) && xcase[0] == c;
}

int Dchar::toLower(dchar c)
{
    wchar_t xcase[2];

    wchar_case(c, xcase);
    return xcase[1];
}

int Dchar::toUpper(dchar c)
{
    wchar_t xcase[2];

    wchar_case(c, xcase);
    return xcase[0];
}

#endif

// Converts a char string to Unicode

dchar *Dchar::dup(char *p)
{
    dchar *s;
    size_t len;

    if (!p)
	return NULL;
    len = strlen(p);
    s = (dchar *)mem.malloc((len + 1) * sizeof(dchar));
    for (unsigned i = 0; i < len; i++)
    {
	s[i] = (dchar)(p[i] & 0xFF);
    }
    s[len] = 0;
    return s;
}

dchar *Dchar::memchr(dchar *p, int c, int count)
{
    int u;

    for (u = 0; u < count; u++)
    {
	if (p[u] == c)
	    return p + u;
    }
    return NULL;
}

#if defined(_WIN32) && defined(__DMC__)
__declspec(naked)
unsigned Dchar::calcHash(const dchar *str, unsigned len)
{
    __asm
    {
	mov	ECX,4[ESP]
	mov	EDX,8[ESP]
	xor	EAX,EAX
	test	EDX,EDX
	je	L92

LC8:	cmp	EDX,1
	je	L98
	cmp	EDX,2
	je	LAE

	add	EAX,[ECX]
//	imul	EAX,EAX,025h
	lea	EAX,[EAX][EAX*8]
	add	ECX,4
	sub	EDX,2
	jmp	LC8

L98:	mov	DX,[ECX]
	and	EDX,0FFFFh
	add	EAX,EDX
	ret

LAE:	add	EAX,[ECX]
L92:	ret
    }
}
#else
unsigned Dchar::calcHash(const dchar *str, unsigned len)
{
    unsigned hash = 0;

    for (;;)
    {
	switch (len)
	{
	    case 0:
		return hash;

#if SIZEOFDCHAR == 4
	    case 1:
		hash += *str;
		return hash;

	    default:
		hash += *str;
		hash *= 37;
		str++;
		len--;
		break;
#else
	    case 1:
		hash += *(unsigned short *)str;
		return hash;

	    case 2:
		hash += *(unsigned long *)str;
		return hash;

	    default:
		hash += *(long *)str;
		hash *= 37;
		str += 2;
		len -= 2;
		break;
#endif
	}
    }
}
#endif // !(_WIN32 && __DMC__)

unsigned Dchar::icalcHash(const dchar *str, unsigned len)
{
    unsigned hash = 0;

    for (;;)
    {
	switch (len)
	{
	    case 0:
		return hash;

#if SIZEOFDCHAR == 4
	    case 1:
		hash += *str | 0x20;
		return hash;

	    default:
		hash += *str | 0x20;
		hash *= 37;
		str++;
		len--;
		break;
#else
	    case 1:
		hash += *(unsigned short *)str | 0x20;
		return hash;

	    case 2:
		hash += *(unsigned long *)str | 0x200020;
		return hash;

	    default:
		hash += *(unsigned long *)str | 0x200020;
		hash *= 37;
		str += 2;
		len -= 2;
		break;
#endif
	}
    }
}

#elif defined(MCBS)

unsigned Dchar::calcHash(const dchar *str, unsigned len)
{
    unsigned hash = 0;

    while (1)
    {
	switch (len)
	{
	    case 0:
		return hash;

	    case 1:
		hash *= 37;
		hash += *(unsigned char *)str;
		return hash;

	    case 2:
		hash *= 37;
		hash += *(unsigned short *)str;
		return hash;

	    case 3:
		hash *= 37;
		hash += (*(unsigned short *)str << 8) +
			((unsigned char *)str)[2];
		return hash;

	    default:
		hash *= 37;
		hash += *(long *)str;
		str += 4;
		len -= 4;
		break;
	}
    }
}

#elif defined(UTF8)

// Specification is: http://anubis.dkuug.dk/JTC1/SC2/WG2/docs/n1335

char Dchar::mblen[256] =
{
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
    4,4,4,4,4,4,4,4,5,5,5,5,6,6,1,1,
};

dchar *Dchar::dec(dchar *pstart, dchar *p)
{
    while ((p[-1] & 0xC0) == 0x80)
	p--;
    return p;
}

int Dchar::get(dchar *p)
{
    unsigned c;
    unsigned char *q = (unsigned char *)p;

    c = q[0];
    switch (mblen[c])
    {
	case 2:
	    c = ((c    - 0xC0) << 6) |
		 (q[1] - 0x80);
	    break;

	case 3:
	    c = ((c    - 0xE0) << 12) |
		((q[1] - 0x80) <<  6) |
		 (q[2] - 0x80);
	    break;

	case 4:
	    c = ((c    - 0xF0) << 18) |
		((q[1] - 0x80) << 12) |
		((q[2] - 0x80) <<  6) |
		 (q[3] - 0x80);
	    break;

	case 5:
	    c = ((c    - 0xF8) << 24) |
		((q[1] - 0x80) << 18) |
		((q[2] - 0x80) << 12) |
		((q[3] - 0x80) <<  6) |
		 (q[4] - 0x80);
	    break;

	case 6:
	    c = ((c    - 0xFC) << 30) |
		((q[1] - 0x80) << 24) |
		((q[2] - 0x80) << 18) |
		((q[3] - 0x80) << 12) |
		((q[4] - 0x80) <<  6) |
		 (q[5] - 0x80);
	    break;
    }
    return c;
}

dchar *Dchar::put(dchar *p, unsigned c)
{
    if (c <= 0x7F)
    {
	*p++ = c;
    }
    else if (c <= 0x7FF)
    {
	p[0] = 0xC0 + (c >> 6);
	p[1] = 0x80 + (c & 0x3F);
	p += 2;
    }
    else if (c <= 0xFFFF)
    {
	p[0] = 0xE0 + (c >> 12);
	p[1] = 0x80 + ((c >> 6) & 0x3F);
	p[2] = 0x80 + (c & 0x3F);
	p += 3;
    }
    else if (c <= 0x1FFFFF)
    {
	p[0] = 0xF0 + (c >> 18);
	p[1] = 0x80 + ((c >> 12) & 0x3F);
	p[2] = 0x80 + ((c >> 6) & 0x3F);
	p[3] = 0x80 + (c & 0x3F);
	p += 4;
    }
    else if (c <= 0x3FFFFFF)
    {
	p[0] = 0xF8 + (c >> 24);
	p[1] = 0x80 + ((c >> 18) & 0x3F);
	p[2] = 0x80 + ((c >> 12) & 0x3F);
	p[3] = 0x80 + ((c >> 6) & 0x3F);
	p[4] = 0x80 + (c & 0x3F);
	p += 5;
    }
    else if (c <= 0x7FFFFFFF)
    {
	p[0] = 0xFC + (c >> 30);
	p[1] = 0x80 + ((c >> 24) & 0x3F);
	p[2] = 0x80 + ((c >> 18) & 0x3F);
	p[3] = 0x80 + ((c >> 12) & 0x3F);
	p[4] = 0x80 + ((c >> 6) & 0x3F);
	p[5] = 0x80 + (c & 0x3F);
	p += 6;
    }
    else
	assert(0);		// not a UCS-4 character
    return p;
}

unsigned Dchar::calcHash(const dchar *str, unsigned len)
{
    unsigned hash = 0;

    while (1)
    {
	switch (len)
	{
	    case 0:
		return hash;

	    case 1:
		hash *= 37;
		hash += *(unsigned char *)str;
		return hash;

	    case 2:
		hash *= 37;
		hash += *(unsigned short *)str;
		return hash;

	    case 3:
		hash *= 37;
		hash += (*(unsigned short *)str << 8) +
			((unsigned char *)str)[2];
		return hash;

	    default:
		hash *= 37;
		hash += *(long *)str;
		str += 4;
		len -= 4;
		break;
	}
    }
}

#else // ascii

unsigned Dchar::calcHash(const dchar *str, unsigned len)
{
    unsigned hash = 0;

    while (1)
    {
	switch (len)
	{
	    case 0:
		return hash;

	    case 1:
		hash *= 37;
		hash += *(unsigned char *)str;
		return hash;

	    case 2:
		hash *= 37;
		hash += *(unsigned short *)str;
		return hash;

	    case 3:
		hash *= 37;
		hash += (*(unsigned short *)str << 8) +
			((unsigned char *)str)[2];
		return hash;

	    default:
		hash *= 37;
		hash += *(long *)str;
		str += 4;
		len -= 4;
		break;
	}
    }
}

unsigned Dchar::icalcHash(const dchar *str, unsigned len)
{
    unsigned hash = 0;

    while (1)
    {
	switch (len)
	{
	    case 0:
		return hash;

	    case 1:
		hash *= 37;
		hash += *(unsigned char *)str | 0x20;
		return hash;

	    case 2:
		hash *= 37;
		hash += *(unsigned short *)str | 0x2020;
		return hash;

	    case 3:
		hash *= 37;
		hash += ((*(unsigned short *)str << 8) +
			 ((unsigned char *)str)[2]) | 0x202020;
		return hash;

	    default:
		hash *= 37;
		hash += *(long *)str | 0x20202020;
		str += 4;
		len -= 4;
		break;
	}
    }
}

#endif // ascii

#if 0
#include <stdio.h>

void main()
{
    // Print out values to hardcode into Dchar::mblen[]
    int c;
    int s;

    for (c = 0; c < 256; c++)
    {
	s = 1;
	if (c >= 0xC0 && c <= 0xDF)
	    s = 2;
	if (c >= 0xE0 && c <= 0xEF)
	    s = 3;
	if (c >= 0xF0 && c <= 0xF7)
	    s = 4;
	if (c >= 0xF8 && c <= 0xFB)
	    s = 5;
	if (c >= 0xFC && c <= 0xFD)
	    s = 6;

	printf("%d", s);
	if ((c & 15) == 15)
	    printf(",\n");
	else
	    printf(",");
    }
}
#endif

