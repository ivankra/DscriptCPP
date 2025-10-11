/* Digital Mars DMDScript source code.
 * Copyright (c) 2000-2001 by Chromium Communications
 * Copyright (c) 2004-2007 by Digital Mars
 * All Rights Reserved.
 * written by Walter Bright
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
#include <string.h>
#include <stdarg.h>

#include "mem.h"
#include "root.h"

#if 0
struct GC
{
    // For passing to debug code
    static unsigned line;
    static char *file;
};
#endif

#if !_MSC_VER
unsigned GC::line;
char *GC::file;
#endif

void process(unsigned char *input,
	unsigned len, OutBuffer *buf, Array *args);

void usage()
{
    printf(
"Merge arguments into file.\n"
"Use:\n"
"    format infile outfile args...\n"
"Use $0, $1, $2, etc., in infile to substitute in arguments.\n"
"A $$ will be replace with a single $, and a $ followed by any\n"
"other character will be inserted anyway.\n"
	);
}

int main(int argc, char *argv[])
{
    int i;
    Array args;
    char *p;
    char *fin = NULL;
    char *fout = NULL;
    int errors;

    mem.init();
    printf("format 0.00\n");

    if (argc < 3)
    {	usage();
	return EXIT_FAILURE;
    }

    errors = 0;
    args.reserve(argc - 3);
    for (i = 1; i < argc; i++)
    {
	p = argv[i];
	if (i <= 2)
	{
	    if (*p == '-')
	    {
		error(DTEXT("unrecognized switch '%s'"),p);
		errors++;
	    }
	    else if (i == 1)
		fin = argv[1];
	    else
		fout = argv[2];
	}
	else
	    args.push(p);
    }
    if (errors)
	return EXIT_FAILURE;

    File input(fin);
    input.readv();

    OutBuffer buf;
    process(input.buffer, input.len, &buf, &args);

    File output(fout);
    output.setbuffer(buf.data, buf.offset);
    buf.data = NULL;
    output.writev();

    return EXIT_SUCCESS;
}

void process(unsigned char *input, unsigned len, OutBuffer *buf, Array *args)
{
    char *p;
    char *pend;
    unsigned i;

    p = (char *)input;
    pend = p + len;
    while (p < pend)
    {
	switch (*p)
	{
	    case 0x1A:
		return;

	    case '$':
		p++;
		switch (*p)
		{
		    case '$':
			buf->writebyte('$');
			p++;
			break;

		    case '0': case '1': case '2': case '3':
		    case '4': case '5': case '6': case '7':
		    case '8': case '9':
			i = *p - '0';
			if (i < args->dim)
			{
			    buf->writestring((char *)args->data[i]);
			}
			p++;
			break;

		    default:
			buf->writebyte('$');
			break;
		}
		break;

	    default:
		buf->writebyte(*p);
		p++;
		break;
	}
    }
}
