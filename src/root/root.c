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
#include <stdarg.h>
#include <string.h>
#include <assert.h>

#if defined(_MSC_VER)
#include <malloc.h>
#endif

#if defined(_WIN32)
#include <windows.h>
#endif

#if defined(linux)
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <utime.h>
#endif

#include "port.h"
#include "root.h"
#include "dchar.h"
#include "mem.h"
#include "printf.h"

#if 0 //__SC__ //def DEBUG
extern "C" void __cdecl _assert(void *e, void *f, unsigned line)
{
    PRINTF("Assert('%s','%s',%d)\n",e,f,line);
    fflush(stdout);
    *(char *)0 = 0;
}
#endif

/*************************************
 * Convert unicode string to ascii string.
 */

char *unicode2ascii(wchar_t *us)
{
    return unicode2ascii(us, wcslen(us));
}

char *unicode2ascii(wchar_t *us, unsigned len)
{
    char *p = (char *)mem.malloc(len + 1);
    if(p)
    {
	for (unsigned i = 0; i <= len; i++)
	{
	    p[i] = (char) us[i];
	}
    }
    return p;
}

int unicodeIsAscii(wchar_t *us)
{
    return unicodeIsAscii(us, wcslen(us));
}

int unicodeIsAscii(wchar_t *us, unsigned len)
{
    for (unsigned i = 0; i <= len; i++)
    {
	if (us[i] & ~0xFF)	// if high bits set
	    return 0;		// it's not ascii
    }
    return 1;
}


/*************************************
 * Convert ascii string to unicode string.
 */

wchar_t *ascii2unicode(const char *s)
{
    unsigned len = strlen(s);
    wchar_t *p = (wchar_t *)mem.malloc((len + 1) * sizeof(wchar_t));
    if(p)
    {
	for (unsigned i = 0; i <= len; i++)
	    p[i] = (wchar_t) s[i];
    }
    return p;
}


/***********************************
 * Compare length-prefixed strings (bstr).
 */

int bstrcmp(unsigned char *b1, unsigned char *b2)
{
    return (*b1 == *b2 && memcmp(b1 + 1, b2 + 1, *b2) == 0) ? 0 : 1;
}

/***************************************
 * Convert bstr into a malloc'd string.
 */

char *bstr2str(unsigned char *b)
{
    unsigned len = *b;
    char *s = (char *) mem.malloc(len + 1);
    if(s)
    {
	s[len] = 0;
	return (char *)memcpy(s,b + 1,len);
    }
    else
    {
	return 0;
    }
}

/**************************************
 * Print error message and exit.
 */

#if defined(UNICODE)
void error(const dchar *format, ...)
{
    va_list ap;

    va_start(ap, format);
    WPRINTF(L"Error: ");
    VWPRINTF(format, ap);
    va_end( ap );
    WPRINTF(L"\n");

    exit(EXIT_FAILURE);
}

/**************************************
 * Print warning message.
 */

void warning(const wchar_t *format, ...)
{
    va_list ap;

    va_start(ap, format);
    WPRINTF(L"Warning: ");
    VWPRINTF(format, ap);
    va_end( ap );
    WPRINTF(L"\n");
}

/**************************************
 * Print a message.
 */

void message(const wchar_t *format, ...)
{
    va_list ap;

    va_start(ap, format);
    VWPRINTF(format, ap);
}

/**************************************
 * Print a message to a buffer.
 */

int message2buf(wchar_t *buffer,unsigned bufsiz, const wchar_t *format, ...)
{
    va_list ap;

    va_start(ap, format);
    int count = VSWPRINTF(buffer,bufsiz,format, ap);
    return count;
}

#else
void error(const char *format, ...)
{
    va_list ap;

    va_start(ap, format);
    PRINTF("Error: ");
    VPRINTF(format, ap);
    va_end( ap );
    PRINTF("\n");

    exit(EXIT_FAILURE);
}

/**************************************
 * Print warning message.
 */

void warning(const char *format, ...)
{
    va_list ap;

    va_start(ap, format);
    PRINTF("Warning: ");
    VPRINTF(format, ap);
    va_end( ap );
    PRINTF("\n");
}

/**************************************
 * Print a message.
 */

void message(const char *format, ...)
{
    int len = strlen(format);
    char *fbuf = (char *)alloca(len+1);
    int i,o;

    for (o = 0,i = 0; i <= len; i++)
    {
        fbuf[o++] = format[i];
        if ((format[i] == '%') && (format[i+2] == 's') &&
            (format[i+1] == 'l' || format[i+1] == 'L'))
        {
            fbuf[o++] = 's';
            i += 2;
        }
    }

    va_list ap;
    va_start(ap, format);
    VPRINTF(fbuf, ap);
    va_end( ap );
}

/**************************************
 * Print a message to a buffer.
 */

int message2buf(char *buffer,size_t bufsiz,const char *format, ...)
{
    int len = strlen(format);
    char *fbuf = (char *)alloca(len+1);
    int i,o;

    for (o = 0,i = 0; i <= len; i++)
    {
        fbuf[o++] = format[i];
        if ((format[i] == '%') && (format[i+2] == 's') &&
            (format[i+1] == 'l' || format[i+1] == 'L'))
        {
            fbuf[o++] = 's';
            i += 2;
        }
    }

    va_list ap;
    va_start(ap, format);
    int count = snprintf(buffer, bufsiz, fbuf, ap);
    va_end( ap );
    return count;
}

#endif

void error_mem()
{
    error(DTEXT("out of memory"));
}

/****************************** Object ********************************/

void *Object::operator new(size_t m_size)
{
   //WPRINTF(L"Object::new(%d)\n", m_size);
   return mem.malloc(m_size);
}

void *Object::operator new(size_t m_size, Mem *mymem)
{
   //WPRINTF(L"Object::new(%d)\n", m_size);
   return mymem->malloc(m_size);
}

void *Object::operator new(size_t m_size, GC *mygc)
{
   //WPRINTF(L"Object::new(%d)\n", m_size);
   return Mem::operator new(m_size, mygc);
}

void Object::operator delete(void *p)
{
    mem.free(p);
}


int Object::equals(Object *o)
{
    return o == this;
}

unsigned Object::hashCode()
{
    return (int) this;
}

int Object::compare(Object *obj)
{
    return this - obj;
}

void Object::print()
{
    PRINTF("%s %p\n", toChars(), this);
}

char *Object::toChars()
{
    return "Object";
}

dchar *Object::toDchars()
{
#if defined(UNICODE)
    return ascii2unicode(toChars());
#else
    return toChars();
#endif
}

void Object::toBuffer(OutBuffer *b)
{
    b->writestring("Object");
}

void Object::mark()
{
}

void Object::invariant()
{
}

/****************************** String ********************************/

String::String(char *str, int ref)
{
    this->str = ref ? str : mem.strdup(str);
    this->ref = ref;
}

String::~String()
{
    mem.free(str);
}

void String::mark()
{
    mem.mark(str);
}

unsigned String::calcHash(const char *str, unsigned len)
{
    unsigned hash = 0;

    for (;;)
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

unsigned String::calcHash(const char *str)
{
    return calcHash(str, strlen(str));
}

unsigned String::hashCode()
{
    return calcHash(str, strlen(str));
}

unsigned String::len()
{
    return strlen(str);
}

int String::equals(Object *obj)
{
    return strcmp(str,((String *)obj)->str) == 0;
}

int String::compare(Object *obj)
{
    return strcmp(str,((String *)obj)->str);
}

char *String::toChars()
{
    return str;
}

void String::print()
{
#if UNICODE
    WPRINTF(DTEXT("String '%s'\n"),str);
#else
    PRINTF("String '%s'\n",str);
#endif
}


/****************************** FileName ********************************/

FileName::FileName(char *str, int ref)
    : String(str,ref)
{
}

static char *combine(char *path, char *name)
{
    if (!path || !*path)
    {
	return name;
    }

    size_t pathlen = strlen(path);
    size_t namelen = strlen(name);
    char *f = (char *)mem.malloc(pathlen + 1 + namelen + 1);
    if(f)
    {
	memcpy(f, path, pathlen);
#   if defined(linux)
	if (path[pathlen - 1] != '/')
	{
	    f[pathlen] = '/';
	    pathlen++;
	}
#   endif
#   if defined(_WIN32)
	if (path[pathlen - 1] != '\\' && path[pathlen - 1] != ':')
	{
	    f[pathlen] = '\\';
	    pathlen++;
	}
#   endif
	memcpy(f + pathlen, name, namelen + 1);
    }
    return f;
}

FileName::FileName(char *path, char *name)
    : String(combine(path,name),1)
{
}

// Split a path into an Array of paths
Array *FileName::splitPath(const char *path)
{
    char c = 0;				// unnecessary initializer is for VC /W4
    const char *p;
    OutBuffer buf;

    Array *array = new Array();
    if (path)
    {
	p = path;
	do
	{   char instring = 0;

	    while (isspace(*p))		// skip leading whitespace
		p++;
	    buf.reserve(strlen(p) + 1);	// guess size of path
	    for (; ; p++)
	    {
		c = *p;
		switch (c)
		{
		    case '"':
			instring ^= 1;	// toggle inside/outside of string
			continue;

#if defined(MACINTOSH)
		    case ',':
#elif defined(_WIN32)
		    case ';':
#elif defined(linux)
		    case ':':
#endif // MACINTOSH
			p++;
			break;		// note that ; cannot appear as part
					// of a path, quotes won't protect it

		    case 0x1A:		// ^Z means end of file
		    case 0:
			break;

		    case '\r':
			continue;	// ignore carriage returns

		    case ' ':
		    case '\t':		// tabs in filenames?
			if (!instring)	// if not in string
			    break;	// treat as end of path
		    default:
			buf.writeByte(c);
			continue;
		}
		break;
	    }
	    if (buf.offset)		// if path is not empty
	    {
		buf.writeByte(0);	// to asciiz
		array->push(buf.extractData());
	    }
	} while (c);
    }
    return array;
}

unsigned FileName::hashCode()
{
#if defined(linux)
    return String::hashCode();
#elif defined(_WIN32)
    // We need a different hashCode because it must be case-insensitive
    unsigned len = strlen(str);
    unsigned hash = 0;
    unsigned char *s = (unsigned char *)str;

    for (;;)
    {
	switch (len)
	{
	    case 0:
		return hash;

	    case 1:
		hash *= 37;
		hash += *(unsigned char *)s | 0x20;
		return hash;

	    case 2:
		hash *= 37;
		hash += *(unsigned short *)s | 0x2020;
		return hash;

	    case 3:
		hash *= 37;
		hash += ((*(unsigned short *)s << 8) +
			 ((unsigned char *)s)[2]) | 0x202020;
		break;

	    default:
		hash *= 37;
		hash += *(long *)s | 0x20202020;
		s += 4;
		len -= 4;
		break;
	}
    }
#endif // linux
}

int FileName::compare(Object *obj)
{
#if defined(linux)
    return String::compare(obj);
#elif defined(_WIN32)
    return stricmp(str,((FileName *)obj)->str);
#endif // _WIN32
}

int FileName::equals(Object *obj)
{
#if defined(linux)
    return String::equals(obj);
#elif defined(_WIN32)
    return stricmp(str,((FileName *)obj)->str) == 0;
#endif // _WIN32
}

/************************************
 * Return !=0 if absolute path name.
 */

int FileName::absolute(const char *name)
{
#if defined(linux)
    return (*name == '/');
#elif defined(_WIN32)
    return (*name == '\\') ||
	   (*name == '/')  ||
	   (*name && name[1] == ':');
#endif // _WIN32
}

/********************************
 * Return filename extension (read-only).
 * If there isn't one, return NULL.
 */

char *FileName::ext(const char *str)
{
    size_t len = strlen(str);
    char *e = (char *)str + len;

    for (;;)
    {
	switch (*e)
	{   case '.':
		return e + 1;
#if defined(linux)
	    case '/':
	        break;
#elif defined(_WIN32)
	    case '\\':
	    case ':':
		break;
#endif
	    default:
		if (e == str)
		    break;
		e--;
		continue;
	}
	return NULL;
    }
}

char *FileName::ext()
{
    return ext(str);
}

/********************************
 * Return filename name excluding path (read-only).
 */

char *FileName::name(const char *str)
{
    size_t len = strlen(str);
    char *e = (char *)str + len;

    for (;;)
    {
	switch (*e)
	{
#if defined(linux)
	    case '/':
	       return e + 1;
#elif defined(_WIN32)
	    case '\\':
	    case ':':
		return e + 1;
#endif
	    default:
		if (e == str)
		    break;
		e--;
		continue;
	}
	return e;
    }
}

char *FileName::name()
{
    return name(str);
}

/**************************************
 * Replace filename portion of path.
 */

char *FileName::replaceName(char *path, char *name)
{
    if (absolute(name))
    {
	return name;
    }

    char *n = FileName::name(path);

    if (n == path)
    {
	return name;
    }

    size_t pathlen = n - path;
    size_t namelen = strlen(name);
    char *f = (char *)mem.malloc(pathlen + 1 + namelen + 1);
    if(f)
    {
	memcpy(f, path, pathlen);

#   if defined(linux)
	if (path[pathlen - 1] != '/')
	{
	    f[pathlen] = '/';
	    pathlen++;
	}
#   elif defined(_WIN32)
	if (path[pathlen - 1] != '\\' && path[pathlen - 1] != ':')
	{
	    f[pathlen] = '\\';
	    pathlen++;
	}
#   endif

	memcpy(f + pathlen, name, namelen + 1);
    }
    return f;
}

/***************************
 */

FileName *FileName::defaultExt(const char *name, const char *ext)
{
    char *e = FileName::ext(name);
    if (e)				// if already has an extension
    {
	return new FileName((char *)name, 0);
    }

    size_t len = strlen(name);
    size_t extlen = strlen(ext);
    char *s = (char *)alloca(len + 1 + extlen + 1);

    memcpy(s,name,len);
    s[len] = '.';
    memcpy(s + len + 1, ext, extlen + 1);
    return new FileName(s, 0);
}

/***************************
 */

FileName *FileName::forceExt(const char *name, const char *ext)
{
    char *e = FileName::ext(name);
    if (e)				// if already has an extension
    {
	size_t len = e - name;
	size_t extlen = strlen(ext);

	char *s = (char *)alloca(len + extlen + 1);
	memcpy(s,name,len);
	memcpy(s + len, ext, extlen + 1);
	return new FileName(s, 0);
    }
    else
	return defaultExt(name, ext);	// doesn't have one
}

/******************************
 * Return !=0 if extensions match.
 */

int FileName::equalsExt(const char *ext)
{   const char *e;

    e = FileName::ext();
    if (!e && !ext)
    {
	return 1;
    }
    if (!e || !ext)
    {
	return 0;
    }
#if defined(linux)
    return strcmp(e,ext) == 0;
#elif defined(_WIN32)
    return stricmp(e,ext) == 0;
#endif
}

/*************************************
 * Copy file from this to to.
 */

void FileName::CopyTo(FileName *to)
{
    File file(this);

#if defined(linux)
    file.touchtime = mem.malloc(sizeof(struct stat)); // keep same file time
#elif defined(_WIN32)
    file.touchtime = mem.malloc(sizeof(WIN32_FIND_DATAA));	// keep same file time
#endif
    file.readv();
    file.name = to;
    file.writev();
}

/*************************************
 * Search Path for file.
 * Input:
 *	cwd	if !=0, search current directory before searching path
 */

char *FileName::searchPath(Array *path, char *name, int cwd)
{
    if (absolute(name))
    {
	return exists(name) ? name : NULL;
    }
    if (cwd)
    {
	if (exists(name))
	    return name;
    }
    if (path)
    {	unsigned i;

	for (i = 0; i < path->dim; i++)
	{
	    char *p = (char *)path->data[i];
	    char *n = combine(p, name);

	    if (exists(n))
		return n;
	}
    }
    return NULL;
}

int FileName::exists(const char *name)
{
#if defined(linux)
    return 0;

#elif defined(_WIN32)
    DWORD dw = GetFileAttributesA(name);
    int result;

    if (dw == -1L)
    {
	result = 0;
    }
    else if (dw & FILE_ATTRIBUTE_DIRECTORY)
    {
	result = 2;
    }
    else
    {
	result = 1;
    }
    return result;
#endif
}

/****************************** File ********************************/

File::File(FileName *n)
{
    ref = 0;
    buffer = NULL;
    len = 0;
    touchtime = NULL;
    name = n;
}

File::File(char *n)
{
    ref = 0;
    buffer = NULL;
    len = 0;
    touchtime = NULL;
    name = new FileName(n, 0);
}

File::~File()
{
    if (!ref && buffer)
    {
	mem.free(buffer);
    }
    if (touchtime)
    {
	mem.free(touchtime);
    }
}

void File::mark()
{
    mem.mark(buffer);
    mem.mark(touchtime);
    mem.mark(name);
}

/*************************************
 */

int File::read()
{
#if defined(linux)
    ssize_t numread; // must be initialized before any "goto"
    off_t size;
    struct stat buf;

    int result = 0;
    char *name = this->name->toChars();
    //PRINTF("File::read('%s')\n",name);
    int fd = open(name, O_RDONLY);
    if (fd == -1)
    {	result = errno;
	//PRINTF("\topen error, errno = %d\n",errno);
	goto err1;
    }

    if (!ref)
    {
	mem.free(buffer);
    }
    ref = 0;       // we own the buffer now

    //PRINTF("\tfile opened\n");
    if (fstat(fd, &buf))
    {
	error(DTEXT("\tfstat error, errno = %d\n"),errno);
        goto err2;
    }

    size = buf.st_size;
    buffer = (unsigned char *) mem.malloc(size + 2);
    if (!buffer)
    {
	error(DTEXT("\tmalloc error, errno = %d\n"),errno);
	goto err2;
    }

    numread = ::read(fd, buffer, size);
    if (numread != size)
    {
	error(DTEXT("\tread error, errno = %d\n"),errno);
	goto err2;
    }

    if (touchtime)
    {
        memcpy(touchtime, &buf, sizeof(buf));
    }

    if (close(fd) == -1)
    {
	error(DTEXT("\tclose error, errno = %d\n"),errno);
	goto err;
    }

    len = size;

    // Always store a unicode ^Z past end of buffer so scanner has a sentinel
    buffer[size] = 0x1A;
    buffer[size + 1] = 0;
    return 0;

err2:
    close(fd);
err:
    mem.free(buffer);
    buffer = NULL;
    len = 0;

err1:
    result = 1;
    return result;
#elif defined(_WIN32)
    DWORD size;
    DWORD numread;
    HANDLE h;
    int result = 0;
    char *name;

    name = this->name->toChars();
    h = CreateFileA(name,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,
	FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,0);
    if (h == INVALID_HANDLE_VALUE)
	goto err1;

    if (!ref)
    {
	mem.free(buffer);
    }
    ref = 0;

    size = GetFileSize(h,NULL);
    buffer = (unsigned char *) mem.malloc(size + 2);
    if (!buffer)
    {
	goto err2;
    }

    if (ReadFile(h,buffer,size,&numread,NULL) != TRUE)
    {
	goto err2;
    }

    if (numread != size)
    {
	goto err2;
    }

    if (touchtime)
    {
	if (!GetFileTime(h, NULL, NULL, &((WIN32_FIND_DATAA *)touchtime)->ftLastWriteTime))
	{
	    goto err2;
	}
    }

    if (!CloseHandle(h))
    {
	goto err;
    }

    len = size;

    // Always store a unicode ^Z past end of buffer so scanner has a sentinel
    buffer[size] = 0x1A;
    buffer[size + 1] = 0;
    return 0;

err2:
    CloseHandle(h);
err:
    mem.free(buffer);
    buffer = NULL;
    len = 0;

err1:
    result = 1;
    return result;
#endif
}

/*********************************************
 * Write a file.
 * Returns:
 *	0	success
 */

int File::write()
{
#if defined(linux)
    int fd;
    ssize_t numwritten;
    char *name;

    name = this->name->toChars();
    fd = open(name, O_CREAT | O_WRONLY | O_TRUNC, 0660);
    if (fd == -1)
    {
	goto err;
    }

    numwritten = ::write(fd, buffer, len);
    if (len != (unsigned)numwritten)
    {
	goto err2;
    }

    if (close(fd) == -1)
    {
	goto err;
    }

    if (touchtime)
    {
	struct utimbuf ubuf;

        ubuf.actime = ((struct stat *)touchtime)->st_atime;
        ubuf.modtime = ((struct stat *)touchtime)->st_mtime;
	if (utime(name, &ubuf))
	{
	    goto err;
	}
    }
    return 0;

err2:
    close(fd);
err:
    return 1;
#elif defined(_WIN32)
    char *name = this->name->toChars();
    HANDLE h = CreateFileA(name,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,
	FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,NULL);

    if (h == INVALID_HANDLE_VALUE)
    {
	goto err;
    }

    DWORD numwritten;
    if (WriteFile(h,buffer,len,&numwritten,NULL) != TRUE)
    {
	goto err2;
    }

    if (len != numwritten)
    {
	goto err2;
    }

    if (touchtime)
    {
        SetFileTime(h, NULL, NULL, &((WIN32_FIND_DATAA *)touchtime)->ftLastWriteTime);
    }

    if (!CloseHandle(h))
    {
	goto err;
    }
    return 0;

err2:
    CloseHandle(h);
err:
    return 1;
#endif
}

/*********************************************
 * Append to a file.
 * Returns:
 *	0	success
 */

int File::append()
{
#if defined(linux)
    return 1;
#elif defined(_WIN32)
    char *name = this->name->toChars();
    HANDLE h = CreateFileA(name,GENERIC_WRITE,0,NULL,OPEN_ALWAYS,
	FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,NULL);
    if (h == INVALID_HANDLE_VALUE)
	goto err;

#if 1
    SetFilePointer(h, 0, NULL, FILE_END);
#else // INVALID_SET_FILE_POINTER doesn't seem to have a definition
    if (SetFilePointer(h, 0, NULL, FILE_END) == INVALID_SET_FILE_POINTER)
	goto err;
#endif

    DWORD numwritten;
    if (WriteFile(h,buffer,len,&numwritten,NULL) != TRUE)
    {
	goto err2;
    }

    if (len != numwritten)
    {
	goto err2;
    }

    if (touchtime)
    {
        SetFileTime(h, NULL, NULL, &((WIN32_FIND_DATAA *)touchtime)->ftLastWriteTime);
    }

    if (!CloseHandle(h))
    {
	goto err;
    }
    return 0;

err2:
    CloseHandle(h);
err:
    return 1;
#endif
}

/**************************************
 */

void File::readv()
{
    if (read())
    {
	error(DTEXT("Error reading file '%s'\n"),name->toDchars());
    }
}

void File::writev()
{
    if (write())
    {
	error(DTEXT("Error writing file '%s'\n"),name->toDchars());
    }
}

void File::appendv()
{
    if (write())
    {
	error(DTEXT("Error appending to file '%s'\n"),name->toDchars());
    }
}

/*******************************************
 * Return !=0 if file exists.
 *	0:	file doesn't exist
 *	1:	normal file
 *	2:	directory
 */

int File::exists()
{
#if defined(linux)
    return 0;
#elif defined(_WIN32)
    DWORD dw;
    char *name = this->name->toChars();

    if (touchtime)
    {
	dw = ((WIN32_FIND_DATAA *)touchtime)->dwFileAttributes;
    }
    else
    {
	dw = GetFileAttributesA(name);
    }

    int result;
    if (dw == -1L)
    {
	result = 0;
    }
    else if (dw & FILE_ATTRIBUTE_DIRECTORY)
    {
	result = 2;
    }
    else
    {
	result = 1;
    }
    return result;
#endif
}

Array *File::match(char *n)
{
    return match(new FileName(n, 0));
}

Array *File::match(FileName *n)
{
#if defined(linux)
    return NULL;
#elif defined(_WIN32)
    Array *a = new Array();
    char *c = n->toChars();
    char *name = n->name();
    WIN32_FIND_DATAA fileinfo;
    HANDLE h = FindFirstFileA(c,&fileinfo);

    if (h != INVALID_HANDLE_VALUE)
    {
	do
	{
	    // Glue path together with name
	    char *fn = (char *)mem.malloc(name - c + strlen(fileinfo.cFileName) + 1);
	    if(fn)
	    {
		memcpy(fn, c, name - c);
		strcpy(fn + (name - c), fileinfo.cFileName);
		File *f = new File(fn);
		f->touchtime = mem.malloc(sizeof(WIN32_FIND_DATAA));
		if(f->touchtime)
		{
		    memcpy(f->touchtime, &fileinfo, sizeof(fileinfo));
		}
		else
		{
		    error_mem();
		    return NULL;
		}

		a->push(f);
	    }
	    else
	    {
		error_mem();
		return NULL;
	    }
	} while (FindNextFileA(h,&fileinfo) != FALSE);
	FindClose(h);
    }
    return a;
#endif
}

int File::compareTime(File *f)
{
#if defined(linux)
    return 0;
#elif defined(_WIN32)
    if (!touchtime)
	stat();
    if (!f->touchtime)
	f->stat();
    return CompareFileTime(&((WIN32_FIND_DATAA *)touchtime)->ftLastWriteTime, &((WIN32_FIND_DATAA *)f->touchtime)->ftLastWriteTime);
#endif
}

void File::stat()
{
#if defined(linux)
    if (!touchtime)
    {
	touchtime = mem.calloc(1, sizeof(struct stat));
    }
#elif defined(_WIN32)
    if (!touchtime)
    {
	touchtime = mem.calloc(1, sizeof(WIN32_FIND_DATAA));
    }
    HANDLE h = FindFirstFileA(name->toChars(),(WIN32_FIND_DATAA *)touchtime);
    if (h != INVALID_HANDLE_VALUE)
    {
	FindClose(h);
    }
#endif
}

void File::checkoffset(size_t offset, size_t nbytes)
{
    if (offset > len || offset + nbytes > len)
    {
	error(DTEXT("Corrupt file '%s': offset x%x off end of file"),toChars(),offset);
    }
}

char *File::toChars()
{
    return name->toChars();
}

void File::toUnicode()
{
    if (buffer)
    {
	// Unicode files start with 0xFEFF
	// (byte swapped Unicode files start with 0xFFFE, not supported)
	if (len >= sizeof(wchar_t) && *((unsigned short *)buffer) == 0xFEFF)
	{
	    if (len & (sizeof(wchar_t) - 1))
	    {
		error(DTEXT("Unicode file has odd size"));
	    }
	    buffer += sizeof(wchar_t);
	    len -= sizeof(wchar_t);
	}
	else
	{
	    wchar_t *base = (wchar_t *)mem.malloc((len + 1) * sizeof(wchar_t));
	    if(base)
	    {
		for (unsigned i = 0; i < len; i++)
		{
		    base[i] = (wchar_t)(buffer[i] & 0xFF);
		}
		base[len] = 0x1A;		// sentinel at end
		buffer = (unsigned char *)base;
		len *= sizeof(wchar_t);
	    }
	    else
	    {
		error_mem();
	    }
	}
    }
}


/************************* OutBuffer *************************/

OutBuffer::OutBuffer()
{
    data = NULL;
    offset = 0;
    size = 0;
}

OutBuffer::~OutBuffer()
{
    mem.free(data);
}

void *OutBuffer::extractData()
{
    void *p = (void *)data;
    data = NULL;
    offset = 0;
    size = 0;
    return p;
}

void OutBuffer::mark()
{
    mem.mark(data);
}

void OutBuffer::reset()
{
    offset = 0;
}

void OutBuffer::setsize(unsigned size)
{
    offset = size;
}

void OutBuffer::write(const void *data, unsigned nbytes)
{
    reserve(nbytes);
    memcpy(this->data + offset, data, nbytes);
    offset += nbytes;
}

void OutBuffer::writebstring(unsigned char *string)
{
    write(string,*string + 1);
}

void OutBuffer::writestring(const char *string)
{
    write(string,strlen(string));
}

void OutBuffer::writedstring(const char *string)
{
#if defined(UNICODE)
    for (; *string; string++)
    {
	writedchar(*string);
    }
#else
    write(string,strlen(string));
#endif
}

void OutBuffer::writedstring(const wchar_t *string)
{
#if defined(UNICODE)
    write(string,wcslen(string) * sizeof(wchar_t));
#else
    for (; *string; string++)
    {
	writedchar(*string);
    }
#endif
}

void OutBuffer::prependstring(char *string)
{
    unsigned len = strlen(string);
    reserve(len);
    memmove(data + len, data, offset);
    memcpy(data, string, len);
    offset += len;
}

void OutBuffer::writenl()
{
#if defined(_WIN32)
#if defined(UNICODE)
    write4(0x000A000D);		// newline is CR,LF on Microsoft OS's
#else
    writeword(0x0A0D);		// newline is CR,LF on Microsoft OS's
#endif // UNICODE
#else
#if defined(UNICODE)
#if SIZEOFDCHAR == 4
    write4('\n');
#else
    writeword('\n');
#endif
#else
    writeByte('\n');
#endif // UNICODE
#endif // !_WIN32
}

void OutBuffer::writeByte(unsigned b)
{
    reserve(1);
    this->data[offset] = (unsigned char)b;
    offset++;
}

void OutBuffer::writedchar(unsigned b)
{
    reserve(Dchar_mbmax * sizeof(dchar));
    offset = (unsigned char *)Dchar::put((dchar *)(this->data + offset), (dchar)b) -
		this->data;
}

void OutBuffer::tab(unsigned level,unsigned sp)
{
    unsigned len = level*sp;
    reserve(len);
    while(len--)
    {
	this->data[offset] = ' ';
	offset++;
    }
}

void OutBuffer::prependbyte(unsigned b)
{
    reserve(1);
    memmove(data + 1, data, offset);
    data[0] = (unsigned char)b;
    offset++;
}

void OutBuffer::writeword(unsigned w)
{
    reserve(2);
    *(unsigned short *)(this->data + offset) = (unsigned short)w;
    offset += 2;
}

void OutBuffer::write4(unsigned w)
{
    reserve(4);
    *(unsigned *)(this->data + offset) = w;
    offset += 4;
}

void OutBuffer::write(OutBuffer *buf)
{
    if (buf)
    {	reserve(buf->offset);
	memcpy(data + offset, buf->data, buf->offset);
	offset += buf->offset;
    }
}

void OutBuffer::write(Object *obj)
{
    if (obj)
    {
	writestring(obj->toChars());
    }
}

void OutBuffer::fill0(unsigned nbytes)
{
    reserve(nbytes);
    memset(data + offset,0,nbytes);
    offset += nbytes;
}

void OutBuffer::align(unsigned size)
{
    unsigned nbytes = ((offset + size - 1) & ~(size - 1)) - offset;
    fill0(nbytes);
}

void OutBuffer::vprintf(const char *format, va_list args)
{
    char buffer[128];
    char *p = buffer;
    unsigned psize = sizeof(buffer);
    int count;

    for (;;)
    {
#if defined(linux)
	count = VSPRINTF(p,psize,format,args);
	if (count == -1)
	    psize *= 2;
	else if ((unsigned)count >= psize)
	    psize = count + 1;
	else
	    break;
#elif defined(_WIN32)
	count = VSPRINTF(p,psize,format,args);
	if (count != -1)
	    break;
	psize *= 2;
#endif
	p = (char *) alloca(psize);	// buffer too small, try again with larger size
    }
    write(p,count);
}

#if defined(UNICODE)
void OutBuffer::vprintf(const wchar_t *format, va_list args)
{
    dchar buffer[128];
    dchar *p = buffer;
    unsigned psize = sizeof(buffer) / sizeof(buffer[0]);
    int count;

    for (;;)
    {
#if defined(linux)
	count = VSWPRINTF(p, psize, (wchar_t*)format, args);
	if (count == -1)
	    psize *= sizeof(wchar_t);
	else if ((unsigned)count >= psize)
	    psize = count + 1;
	else
	    break;
#elif defined(_WIN32)
	count = VSWPRINTF(p,psize,format,args);
	if (count != -1)
	    break;
	psize *= 2;
#endif // _WIN32
	p = (dchar *) alloca(psize * sizeof(wchar_t));	// buffer too small, try again with larger size
    }

    write(p,count * sizeof(wchar_t));
}
#endif // UNICODE

void OutBuffer::printf(const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    vprintf(format,ap);
    va_end(ap);
}

#if defined(UNICODE)
void OutBuffer::printf(const wchar_t *format, ...)
{
    va_list ap;
    va_start(ap, format);
    vprintf(format,ap);
    va_end(ap);
}
#endif // UNICODE

void OutBuffer::bracket(char left, char right)
{
    reserve(2);
    memmove(data + 1, data, offset);
    data[0] = left;
    data[offset + 1] = right;
    offset += 2;
}

void OutBuffer::spread(unsigned offset, unsigned nbytes)
{
    reserve(nbytes);
    memmove(data + offset + nbytes, data + offset,
	this->offset - offset);
    this->offset += nbytes;
}

char *OutBuffer::toChars()
{
    writeByte(0);
    return (char *)data;
}

/********************************* Array ****************************/

#if ARRAY_INVARIANT

void Array::invariant()
{
    assert(signature == ARRAY_SIGNATURE);
    assert(dim <= allocdim);
    assert((allocdim && data) || (allocdim == 0 && data == NULL));
}

#endif

int Array::reserve(unsigned nentries)
{
    invariant();
    //PRINTF("Array::reserve: size = %d, offset = %d, nbytes = %d\n", size, offset, nbytes);
    if ((allocdim - dim) < nentries)
    {
	GC_LOG();
	long new_allocdim = dim + nentries;
	void **newdata = (void **)mem.realloc(data, new_allocdim * sizeof(*data));
	if (!newdata)
	{
	    return 1;
	}
	data = newdata;
	allocdim = new_allocdim;
    }
    return 0;
}

int Array::setDim(unsigned newdim)
{
    invariant();
    if (dim < newdim)
    {
	if (reserve(newdim - dim))
	    return 1;
    }
    dim = newdim;
    return 0;
}

int Array::fixDim()
{
    invariant();
    if (dim != allocdim)
    {	void **newdata;

	newdata = (void **)mem.realloc(data, dim * sizeof(*data));
	if (!newdata)
	{
	    return 1;
	}
	data = newdata;
	allocdim = dim;
    }
    return 0;
}

int Array::shift(void *ptr)
{
    invariant();
    if (reserve(1))
    {
	return 1;
    }

    memmove(data + 1, data, dim * sizeof(*data));
    data[0] = ptr;
    dim++;
    return 0;
}

int Array::insert(unsigned index, void *ptr)
{
    invariant();
#if ARRAY_INVARIANT
    assert(0 <= index && index < dim);
#endif
    if (reserve(1))
    {
	return 1;
    }

    memmove(data + index + 1, data + index, (dim - index) * sizeof(*data));
    data[index] = ptr;
    dim++;
    return 0;
}


/***********************************
 * Append array a to this array.
 */

int Array::append(Array *a)
{
    invariant();
    a->invariant();

    if (a)
    {   unsigned d;

	d = a->dim;
	if (reserve(d))
	    return 1;
	memcpy(data + dim, a->data, d * sizeof(data[0]));
	dim += d;
    }
    return 0;
}

void Array::remove(unsigned i)
{
    invariant();
#if ARRAY_INVARIANT
    assert(0 <= i && i < dim);
#endif
    memcpy(data + i, data + i + 1, (dim - i) * sizeof(data[0]));
    dim--;
}

char *Array::toChars()
{
    invariant();
    char **buf = (char **)alloca(dim * sizeof(char *));

    unsigned len = 2;
    unsigned u;
    for (u = 0; u < dim; u++)
    {
	buf[u] = ((Object *)data[u])->toChars();
	len += strlen(buf[u]) + 1;
    }

    char *str = (char *)mem.malloc(len);
    if(str)
    {
	str[0] = '[';
	char *p = str + 1;
	for (u = 0; u < dim; u++)
	{
	    if (u)
		*p++ = ',';
	    len = strlen(buf[u]);
	    memcpy(p,buf[u],len);
	    p += len;
	}
	*p++ = ']';
	*p = 0;
    }
    return str;
}

void *Array::tos()
{
    invariant();
    return dim ? data[dim - 1] : NULL;
}

int __cdecl Array_sort_compare(const void *x, const void *y)
{
    Object *ox = *(Object **)x;
    Object *oy = *(Object **)y;

    return ox->compare(oy);
}

void Array::sort()
{
    invariant();
    if (dim)
    {
	qsort(data, dim, sizeof(Object *), Array_sort_compare);
    }
}

/********************************* Bits ****************************/

Bits::Bits()
{
    data = NULL;
    bitdim = 0;
    allocdim = 0;
}

Bits::~Bits()
{
    mem.free(data);
}

void Bits::mark()
{
    mem.mark(data);
}

void Bits::resize(unsigned bitdim)
{
    unsigned allocdim = (bitdim + 31) / 32;
    data = (unsigned *)mem.realloc(data, allocdim * sizeof(data[0]));
    if (this->allocdim < allocdim)
    {
	memset(data + this->allocdim, 0, (allocdim - this->allocdim) * sizeof(data[0]));
    }

    // Clear other bits in last word
    unsigned mask = (1 << (bitdim & 31)) - 1;
    if (mask)
    {
	data[allocdim - 1] &= ~mask;
    }

    this->bitdim = bitdim;
    this->allocdim = allocdim;
}

void Bits::set(unsigned bitnum)
{
    data[bitnum / 32] |= 1 << (bitnum & 31);
}

void Bits::clear(unsigned bitnum)
{
    data[bitnum / 32] &= ~(1 << (bitnum & 31));
}

int Bits::test(unsigned bitnum)
{
    return data[bitnum / 32] & (1 << (bitnum & 31));
}

void Bits::set()
{
    memset(data, ~0, allocdim * sizeof(data[0]));

    // Clear other bits in last word
    unsigned mask = (1 << (bitdim & 31)) - 1;
    if (mask)
    {
	data[allocdim - 1] &= ~mask;
    }
}

void Bits::clear()
{
    memset(data, 0, allocdim * sizeof(data[0]));
}

void Bits::copy(Bits *from)
{
    assert(bitdim == from->bitdim);
    memcpy(data, from->data, allocdim * sizeof(data[0]));
}

Bits *Bits::clone()
{
    Bits *b = new Bits();
    b->resize(bitdim);
    b->copy(this);
    return b;
}

void Bits::sub(Bits *b)
{
    for (unsigned u = 0; u < allocdim; u++)
    {
	data[u] &= ~b->data[u];
    }
}















