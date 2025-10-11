/* Digital Mars DMDScript source code.
 * Copyright (c) 1990-2007 by Digital Mars
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

//_ response.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "root.h"
#include "mem.h"

/*********************************
 * Expand any response files in command line.
 * Response files are arguments that look like:
 *   @NAME
 * The name is first searched for in the environment. If it is not
 * there, it is searched for as a file name.
 * Arguments are separated by spaces, tabs, or newlines. These can be
 * imbedded within arguments by enclosing the argument in '' or "".
 * Recursively expands nested response files.
 *
 * To use, put the line:
 *   response_expand(&argc,&argv);
 * as the first executable statement in main(int argc, char **argv).
 * argc and argv are adjusted to be the new command line arguments
 * after response file expansion.
 *
 * Returns:
 *   0   success
 *   !=0   failure (argc, argv unchanged)
 */

struct Narg
{
    int argc;	   // arg count
    int argvmax;   // dimension of nargv[]
    char **argv;
};

static int addargp(struct Narg *n,char *p)
{
    /* The 2 is to always allow room for a NULL argp at the end   */
    if (n->argc + 2 >= n->argvmax)
    {
        n->argvmax = n->argc + 2;
        n->argv = (char **) mem.realloc(n->argv,n->argvmax * sizeof(char *));
        if (!n->argv)
            return 1;
    }
    n->argv[n->argc++] = p;
    return 0;
}

int my_response_expand(int *pargc, char ***pargv)
{
    struct Narg n;
    int i;
    char *cp;
    int recurse = 0;

    n.argc = 0;
    n.argvmax = 0;      /* dimension of n.argv[]      */
    n.argv = NULL;
    for (i = 0; i < *pargc; ++i)
    {
        cp = (*pargv)[i];
        if (*cp == '@')
        {
            char *buffer;
            char *bufend;
            char *p;

            cp++;
            p = getenv(cp);
            if (p)
            {
                buffer = mem.strdup(p);
                if (!buffer)
                    goto noexpand;
                bufend = buffer + strlen(buffer);
            }
            else
	    {
		File f(cp);
		if (f.read())
		    goto noexpand;
		buffer = (char *)f.buffer;
		bufend = buffer + f.len;
		f.buffer = NULL;
		f.len = 0;
	    }

            // The logic of this should match that in setargv()

            for (p = buffer; p < bufend; p++)
            {
                char *d;
                char c,lastc;
                unsigned char instring;
                int num_slashes,non_slashes;

                switch (*p)
                {
                    case 26:      /* ^Z marks end of file      */
                        goto L2;

                    case 0xD:
                    case 0:
                    case ' ':
                    case '\t':
                    case '\n':
                        continue;   // scan to start of argument

                    case '@':
                        recurse = 1;
                    default:      /* start of new argument   */
                        if (addargp(&n,p))
                            goto noexpand;
                        instring = 0;
                        c = 0;
                        num_slashes = 0;
                        for (d = p; ; p++)
                        {
                            lastc = c;
                            if (p >= bufend)
                                goto Lend;
                            c = *p;
                            switch (c)
                            {
                                case '"':
                                    /*
                                        Yes this looks strange,but this is so that we are
                                        MS Compatible, tests have shown that:
                                        \\\\"foo bar"  gets passed as \\foo bar
                                        \\\\foo  gets passed as \\\\foo
                                        \\\"foo gets passed as \"foo
                                        and \"foo gets passed as "foo in VC!
                                     */
                                    non_slashes = num_slashes % 2;
                                    num_slashes = num_slashes / 2;
                                    for (; num_slashes > 0; num_slashes--)
                                    {
                                        d--;
                                        *d = '\0';
                                    }

                                    if (non_slashes)
                                    {
                                        *(d-1) = c;
                                    }
                                    else
                                    {
                                        instring ^= 1;
                                    }
                                    break;
                                case 26:
                            Lend:
                                    *d = 0;      // terminate argument
                                    goto L2;

                                case 0xD:      // CR
                                    c = lastc;
                                    continue;      // ignore

                                case '@':
                                    recurse = 1;
                                    goto Ladd;

                                case ' ':
                                case '\t':
                                    if (!instring)
                                    {
                                case '\n':
                                case 0:
                                        *d = 0;      // terminate argument
                                        goto Lnextarg;
                                    }
                                default:
                                Ladd:
                                    if (c == '\\')
                                        num_slashes++;
                                    else
                                        num_slashes = 0;
                                    *d++ = c;
                                    break;
                            }
#ifdef _MBCS
                            if (_istlead (c)) {
                                *d++ = *++p;
                                if (*(d - 1) == '\0') {
                                    d--;
                                    goto Lnextarg;
                                }
                            }
#endif
                        }
                    break;
                }
            Lnextarg:
                ;
            }
        L2:
            ;
        }
        else if (addargp(&n,(*pargv)[i]))
            goto noexpand;
    }
    n.argv[n.argc] = NULL;
    if (recurse)
    {
        /* Recursively expand @filename   */
        if (my_response_expand(&n.argc,&n.argv))
            goto noexpand;
    }
    *pargc = n.argc;
    *pargv = n.argv;
    return 0;            /* success         */

noexpand:            /* error         */
    mem.free(n.argv);
    /* any file buffers are not free'd, but doesn't matter   */
    return 1;
}
