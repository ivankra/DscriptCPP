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
#include <string.h>

#include "dchar.h"
#include "root.h"

#include "dscript.h"
#include "identifier.h"
#include "dobject.h"

#if 1

void Identifier::toBuffer(OutBuffer *buf)
{
    buf->writedstring(toDchars());
}

int Identifier::equals(Identifier *id)
{
    return this == id || Lstring::cmp(toLstring(), id->toLstring()) == 0;
}


#else

Identifier::Identifier(Lstring *string)
{
    this->string = string;
}

unsigned Identifier::hashCode()
{
#if ASCIZ
    return Vstring::calcHash(string->toDchars());
#else
    return Vstring::calcHash(string);
#endif
}

int Identifier::equals(Object *o)
{   Identifier *id;

    return this == o ||
	((id = dynamic_cast<Identifier *>(o)) != NULL &&
	 Lstring::cmp(string, id->string) == 0
	);
}

int Identifier::compare(Object *o)
{   int c;
    Identifier *id;

    if (this == o)
	c = 0;
    else if ((id = dynamic_cast<Identifier *>(o)) != NULL)
	c = Lstring::cmp(string, id->string);
    else
	c = Dchar::cmp(string->toDchars(), o->toDchars());
    return c;
}

void Identifier::print()
{
#if defined UNICODE
    WPRINTF(L"%ls", toDchars());
#else
    PRINTF("%s", toDchars());
#endif
}

void Identifier::toBuffer(OutBuffer *buf)
{
    buf->writedstring(toDchars());
}


#endif
