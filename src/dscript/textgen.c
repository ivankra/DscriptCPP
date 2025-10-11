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

// Program to generate string files in dscript data structures.
// Saves much tedious typing, and eliminates typo problems.
// Generates:
//	text.h
//	text.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

struct Msgtable
{
	char *name;
	int value;
	char *ident;
};

Msgtable msgtable[] =
{
	{ "" },
	{ "source" },
	{ "global" },
	{ "ignoreCase" },
	{ "multiline" },
	{ "lastIndex" },
	{ "input" },
	{ "lastMatch" },
	{ "lastParen" },
	{ "leftContext" },
	{ "rightContext" },
	{ "prototype" },
	{ "constructor" },
	{ "toString" },
	{ "toLocaleString" },
	{ "toSource" },
	{ "valueOf" },
	{ "message" },
	{ "description" },
	{ "Error" },
	{ "name" },
	{ "length" },
	{ "NaN" },
	{ "Infinity" },
	{ "-Infinity", 0, "negInfinity" },
	{ "[object]", 0, "bobjectb" },
	{ "undefined" },
	{ "null" },
	{ "true" },
	{ "false" },
	{ "object" },
	{ "string" },
	{ "number" },
	{ "boolean" },
	{ "Object" },
	{ "String" },
	{ "Number" },
	{ "Boolean" },
	{ "Date" },
	{ "Array" },
	{ "RegExp" },
	{ "arity" },
	{ "arguments" },
	{ "callee" },
	{ "caller" },			// extension
	{ "EvalError" },
	{ "RangeError" },
	{ "ReferenceError" },
	{ "SyntaxError" },
	{ "TypeError" },
	{ "URIError" },
	{ "this" },
	{ "fromCharCode" },
	{ "charAt" },
	{ "charCodeAt" },
	{ "concat" },
	{ "indexOf" },
	{ "lastIndexOf" },
	{ "localeCompare" },
	{ "match" },
	{ "replace" },
	{ "search" },
	{ "slice" },
	{ "split" },
	{ "substr" },
	{ "substring" },
	{ "toLowerCase" },
	{ "toLocaleLowerCase" },
	{ "toUpperCase" },
	{ "toLocaleUpperCase" },
	{ "hasOwnProperty" },
	{ "isPrototypeOf" },
	{ "propertyIsEnumerable" },
	{ "$1", 0, "dollar1" },
	{ "$2", 0, "dollar2" },
	{ "$3", 0, "dollar3" },
	{ "$4", 0, "dollar4" },
	{ "$5", 0, "dollar5" },
	{ "$6", 0, "dollar6" },
	{ "$7", 0, "dollar7" },
	{ "$8", 0, "dollar8" },
	{ "$9", 0, "dollar9" },
	{ "index" },
	{ "compile" },
	{ "test" },
	{ "exec" },
	{ "MAX_VALUE" },
	{ "MIN_VALUE" },
	{ "NEGATIVE_INFINITY" },
	{ "POSITIVE_INFINITY" },
	{ "-", 0, "dash" },
	{ "toFixed" },
	{ "toExponential" },
	{ "toPrecision" },
	{ "abs" },
	{ "acos" },
	{ "asin" },
	{ "atan" },
	{ "atan2" },
	{ "ceil" },
	{ "cos" },
	{ "exp" },
	{ "floor" },
	{ "log" },
	{ "max" },
	{ "min" },
	{ "pow" },
	{ "random" },
	{ "round" },
	{ "sin" },
	{ "sqrt" },
	{ "tan" },
	{ "E" },
	{ "LN10" },
	{ "LN2" },
	{ "LOG2E" },
	{ "LOG10E" },
	{ "PI" },
	{ "SQRT1_2" },
	{ "SQRT2" },
	{ "parse" },
	{ "UTC" },

	{ "getTime" },
	{ "getYear" },
	{ "getFullYear" },
	{ "getUTCFullYear" },
	{ "getDate" },
	{ "getUTCDate" },
	{ "getMonth" },
	{ "getUTCMonth" },
	{ "getDay" },
	{ "getUTCDay" },
	{ "getHours" },
	{ "getUTCHours" },
	{ "getMinutes" },
	{ "getUTCMinutes" },
	{ "getSeconds" },
	{ "getUTCSeconds" },
	{ "getMilliseconds" },
	{ "getUTCMilliseconds" },
	{ "getTimezoneOffset" },
	{ "getVarDate" },

	{ "setTime" },
	{ "setYear" },
	{ "setFullYear" },
	{ "setUTCFullYear" },
	{ "setDate" },
	{ "setUTCDate" },
	{ "setMonth" },
	{ "setUTCMonth" },
	{ "setDay" },
	{ "setUTCDay" },
	{ "setHours" },
	{ "setUTCHours" },
	{ "setMinutes" },
	{ "setUTCMinutes" },
	{ "setSeconds" },
	{ "setUTCSeconds" },
	{ "setMilliseconds" },
	{ "setUTCMilliseconds" },

	{ "toDateString" },
	{ "toTimeString" },
	{ "toLocaleDateString" },
	{ "toLocaleTimeString" },
	{ "toUTCString" },
	{ "toGMTString" },

	{ ",", 0, "comma" },
	{ "join" },
	{ "pop" },
	{ "push" },
	{ "reverse" },
	{ "shift" },
	{ "sort" },
	{ "splice" },
	{ "unshift" },
	{ "apply" },
	{ "call" },
	{ "function" },

	{ "eval" },
	{ "parseInt" },
	{ "parseFloat" },
	{ "escape" },
	{ "unescape" },
	{ "isNaN" },
	{ "isFinite" },
	{ "decodeURI" },
	{ "decodeURIComponent" },
	{ "encodeURI" },
	{ "encodeURIComponent" },

	{ "print" },
	{ "println" },
	{ "readln" },
	{ "getenv" },
	{ "assert" },

	{ "Function" },
	{ "Math" },

	{ "ActiveXObject" },
	{ "GetObject" },

	{ "0" },
	{ "1" },
	{ "2" },
	{ "3" },
	{ "4" },
	{ "5" },
	{ "6" },
	{ "7" },
	{ "8" },
	{ "9" },

	{ "anchor" },
	{ "big" },
	{ "blink" },
	{ "bold" },
	{ "fixed" },
	{ "fontcolor" },
	{ "fontsize" },
	{ "italics" },
	{ "link" },
	{ "small" },
	{ "strike" },
	{ "sub" },
	{ "sup" },

	{ "Enumerator" },
	{ "item" },
	{ "atEnd" },
	{ "moveNext" },
	{ "moveFirst" },

	{ "VBArray" },
	{ "dimensions" },
	{ "getItem" },
	{ "lbound" },
	{ "toArray" },
	{ "ubound" },

	{ "ScriptEngine" },
	{ "ScriptEngineBuildVersion" },
	{ "ScriptEngineMajorVersion" },
	{ "ScriptEngineMinorVersion" },
	{ "DMDScript", 0, "CHROMEscript" },

	{ "date" },
	{ "unknown" },

//	{ "" },
};

Msgtable errtable[] =
{
    { "DMDScript fatal runtime error: ",                         0, "ERR_RUNTIME_PREFIX" },
    { "No default value for COM object",                         0, "ERR_COM_NO_DEFAULT_VALUE" },
    { "%s does not have a [[Construct]] property",             445, "ERR_COM_NO_CONSTRUCT_PROPERTY" },
    { "argument type mismatch for %s",			       0xD, "ERR_DISP_E_TYPEMISMATCH" },
    { "wrong number of arguments for %s",		     0x1C2, "ERR_DISP_E_BADPARAMCOUNT" },
    { "%s Invoke() fails with COM error %x",                     0, "ERR_COM_FUNCTION_ERROR" },
    { "Dcomobject: %s.%s fails with COM error %x",               0, "ERR_COM_OBJECT_ERROR" },
    { "unrecognized switch '%s'",                                0, "ERR_BAD_SWITCH" },
    { "undefined label '%s' in function '%s'",                   0, "ERR_UNDEFINED_LABEL" },
    { "unterminated /* */ comment",                              0, "ERR_BAD_C_COMMENT" },
    { "<!-- comment does not end in newline",                    0, "ERR_BAD_HTML_COMMENT" },
    { "unsupported char '%c'",                                   0, "ERR_BAD_CHAR_C" },
    { "unsupported char 0x%02x",                                 0, "ERR_BAD_CHAR_X" },
    { "escape hex sequence requires 2 hex digits",               0, "ERR_BAD_HEX_SEQUENCE" },
    { "undefined escape sequence \\\\%c",                        0, "ERR_UNDEFINED_ESC_SEQUENCE" },
    { "string is missing an end quote",                          0, "ERR_STRING_NO_END_QUOTE" },
    { "end of file before end of string",                        0, "ERR_UNTERMINATED_STRING" },
    { "\\\\u sequence must be followed by 4 hex characters",     0, "ERR_BAD_U_SEQUENCE" },
    { "unrecognized numeric literal",                            0, "ERR_UNRECOGNIZED_N_LITERAL" },
    { "Identifier expected in FormalParameterList, not %s",      0, "ERR_FPL_EXPECTED_IDENTIFIER" },
    { "comma expected in FormalParameterList, not %s",           0, "ERR_FPL_EXPECTED_COMMA" },
    { "identifier expected",                                     0, "ERR_EXPECTED_IDENTIFIER" },
    { "found '%s' when expecting '%s'",                       1002, "ERR_EXPECTED_GENERIC" },
    { "identifier expected instead of '%s'",                  1010, "ERR_EXPECTED_IDENTIFIER_PARAM" },
    { "identifier expected following '%s', not '%s'",            0, "ERR_EXPECTED_IDENTIFIER_2PARAM" },
    { "EOF found before closing '}' of block statement",         0, "ERR_UNTERMINATED_BLOCK" },
    { "only one variable can be declared for 'in', not %d",      0, "ERR_TOO_MANY_IN_VARS" },
    { "';' or 'in' expected, not '%s'",                          0, "ERR_IN_EXPECTED" },
    { "label expected after goto, not '%s'",                     0, "ERR_GOTO_LABEL_EXPECTED" },
    { "catch or finally expected following try",                 0, "ERR_TRY_CATCH_EXPECTED" },
    { "found '%s' instead of statement",                         0, "ERR_STATEMENT_EXPECTED" },
    { "expression expected, not '%s'",                           0, "ERR_EXPECTED_EXPRESSION" },
    { "Object literal in initializer",                           0, "ERR_OBJ_LITERAL_IN_INITIALIZER" },
    { "label '%s' is already defined",                           0, "ERR_LABEL_ALREADY_DEFINED" },
    { "redundant case %s",                                       0, "ERR_SWITCH_REDUNDANT_CASE" },
    { "case %s: is not in a switch statement",                   0, "ERR_MISPLACED_SWITCH_CASE" },
    { "redundant default in switch statement",                   0, "ERR_SWITCH_REDUNDANT_DEFAULT" },
    { "default is not in a switch statement",                    0, "ERR_MISPLACED_SWITCH_DEFAULT" },
    { "init statement must be expression or var",                0, "ERR_INIT_NOT_EXPRESSION" },
    { "can only break from within loop or switch",            1019, "ERR_MISPLACED_BREAK" },
    { "continue is not in a loop",                            1020, "ERR_MISPLACED_CONTINUE" },
    { "Statement label '%s' is undefined",                       0, "ERR_UNDEFINED_STATEMENT_LABEL" },
    { "cannot goto into with statement",                         0, "ERR_GOTO_INTO_WITH" },
    { "can only return from within function",                    0, "ERR_MISPLACED_RETURN" },
    { "no expression for throw",                                 0, "ERR_NO_THROW_EXPRESSION" },
    { "'%s' has no semantic routine",                            0, "ERR_NO_SEMANTIC_ROUTINE" },
    { "%s.%s is undefined",                                      0, "ERR_UNDEFINED_OBJECT_SYMBOL" },
    { "Symbol %s.%s is already defined",                         0, "ERR_REDEFINED_OBJECT_SYMBOL" },
    { "%s.%s expects a Number not a %s",                      5001, "ERR_FUNCTION_WANTS_NUMBER" },
    { "%s.%s expects a String not a %s",                      5005, "ERR_FUNCTION_WANTS_STRING" },
    { "%s.%s expects a Date not a %s",                        5006, "ERR_FUNCTION_WANTS_DATE" },
    { "%s %s is undefined and has no Call method",            5007, "ERR_UNDEFINED_NO_CALL2"},
    { "%s %s.%s is undefined and has no Call method",         5007, "ERR_UNDEFINED_NO_CALL3"},
    { "%s.%s expects a Boolean not a %s",                     5010, "ERR_FUNCTION_WANTS_BOOL" },
    { "arg to Array(len) must be 0 .. 2**32-1, not %.16g",    5029, "ERR_ARRAY_LEN_OUT_OF_BOUNDS" },
    { "%s %s out of range",                                      0, "ERR_VALUE_OUT_OF_RANGE" },
    { "TypeError in %s",                                         0, "ERR_TYPE_ERROR" },
    { "Error compiling regular expression",                      0, "ERR_REGEXP_COMPILE" },
    { "%s not transferrable",                                    0, "ERR_NOT_TRANSFERRABLE" },
    { "%s %s cannot convert to Object",                          0, "ERR_CANNOT_CONVERT_TO_OBJECT2" },
    { "%s %s.%s cannot convert to Object",                       0, "ERR_CANNOT_CONVERT_TO_OBJECT3" },
    { "cannot convert %s to Object",                        0x138F, "ERR_CANNOT_CONVERT_TO_OBJECT4" },
    { "cannot assign to %s",                                     0, "ERR_CANNOT_ASSIGN_TO" },
    { "cannot assign %s to %s",                                  0, "ERR_CANNOT_ASSIGN" },
    { "cannot assign to %s.%s",                                  0, "ERR_CANNOT_ASSIGN_TO2" },
    { "cannot assign to function",				 0, "ERR_FUNCTION_NOT_LVALUE"},
    { "RHS of %s must be an Object, not a %s",                   0, "ERR_RHS_MUST_BE_OBJECT" },
    { "can't Put('%s', %s) to a primitive %s",                   0, "ERR_CANNOT_PUT_TO_PRIMITIVE" },
    { "can't Put(%u, %s) to a primitive %s",                     0, "ERR_CANNOT_PUT_INDEX_TO_PRIMITIVE" },
    { "object cannot be converted to a primitive type",          0, "ERR_OBJECT_CANNOT_BE_PRIMITIVE" },
    { "can't Get(%s) from primitive %s(%s)",                     0, "ERR_CANNOT_GET_FROM_PRIMITIVE" },
    { "can't Get(%d) from primitive %s(%s)",                     0, "ERR_CANNOT_GET_INDEX_FROM_PRIMITIVE" },
    { "primitive %s has no Construct method",                 5100, "ERR_PRIMITIVE_NO_CONSTRUCT" },
    { "primitive %s has no Call method",                         0, "ERR_PRIMITIVE_NO_CALL" },
    { "for-in must be on an object, not a primitive",            0, "ERR_FOR_IN_MUST_BE_OBJECT" },
    { "assert() line %d",					 0, "ERR_ASSERT"},
    { "object does not have a [[Call]] property",		 0, "ERR_OBJECT_NO_CALL"},
    { "%s: %s",							 0, "ERR_S_S"},
    { "no Default Put for object",				 0, "ERR_NO_DEFAULT_PUT"},
    { "%s does not have a [[Construct]] property",		 0, "ERR_S_NO_CONSTRUCT"},
    { "%s does not have a [[Call]] property",			 0, "ERR_S_NO_CALL"},
    { "%s does not have a [[HasInstance]] property",		 0, "ERR_S_NO_INSTANCE"},
    { "length property must be an integer",			 0, "ERR_LENGTH_INT"},
    { "Array.prototype.toLocaleString() not transferrable",	 0, "ERR_TLS_NOT_TRANSFERRABLE"},
    { "Function.prototype.toString() not transferrable",	 0, "ERR_TS_NOT_TRANSFERRABLE"},
    { "Function.prototype.apply(): argArray must be array or arguments object", 0, "ERR_ARRAY_ARGS"},
    { ".prototype must be an Object, not a %s",			 0, "ERR_MUST_BE_OBJECT"},
    { "VBArray expected, not a %s",				 0, "ERR_VBARRAY_EXPECTED"},
    { "VBArray subscript out of range",				 0, "ERR_VBARRAY_SUBSCRIPT"},
    { "Type mismatch",						 0, "ERR_ACTIVEX"},
    { "no property %s",						 0, "ERR_NO_PROPERTY"},
    { "Put of %s failed",					 5, "ERR_PUT_FAILED"},
    { "Get of %s failed",					 0, "ERR_GET_FAILED"},
    { "argument not a collection",				 0, "ERR_NOT_COLLECTION"},

// COM error messages
    { "Unexpected",						 0, "ERR_E_UNEXPECTED"},
};

int main()
{
    FILE *fp;
    unsigned i;
#if defined UNICODE
    char L[] = "L";
#else
    char L[] = "";
#endif

    fp = fopen("text.h","w");
    if (!fp)
    {
	printf("can't open text.h\n");
	exit(EXIT_FAILURE);
    }

    fprintf(fp, "// File generated by textgen.c\n");
    fprintf(fp, "#ifndef TEXT2_H\n");
    fprintf(fp, "#define TEXT2_H 1\n");

    fprintf(fp, "//\n");
    fprintf(fp, "// *** BASIC TEXT / KEYWORDS ***\n");
    fprintf(fp, "//\n");
    for (i = 0; i < sizeof(msgtable) / sizeof(msgtable[0]); i++)
    {	char *id = msgtable[i].ident;

	if (!id)
	    id = msgtable[i].name;
	fprintf(fp,"extern d_string TEXT_%s;\n", id);
    }

    fprintf(fp, "//\n");
    fprintf(fp, "// *** ERROR MESSAGES ***\n");
    fprintf(fp, "//\n");
    fprintf(fp, "enum ERRMSGS {\n");
    for (i = 0; i < sizeof(errtable) / sizeof(errtable[0]); i++)
    {	char *id = errtable[i].ident;

	if (!id)
	    id = errtable[i].name;
	fprintf(fp,"\t%s = %d,\n", id, i);
    }
    fprintf(fp, "};\n");
    fprintf(fp, "extern char * errmsgtbl[];\n");
    fprintf(fp, "extern unsigned short errcodtbl[];\n");

    fprintf(fp, "#endif\n");

    fclose(fp);

    fp = fopen("text.c","w");
    if (!fp)
    {	printf("can't open text.h\n");
	exit(EXIT_FAILURE);
    }

    fprintf(fp, "// File generated by textgen.c\n");
#if defined UNICODE
    fprintf(fp, "#include <wchar.h>\n");
#endif
    fprintf(fp, "#include \"dchar.h\"\n");
    fprintf(fp, "#include \"lstring.h\"\n");
    fprintf(fp, "#include \"dscript.h\"\n");
    fprintf(fp, "#include \"text.h\"\n");

    fprintf(fp, "//\n");
    fprintf(fp, "// *** BASIC TEXT / KEYWORDS ***\n");
    fprintf(fp, "//\n");
    for (i = 0; i < sizeof(msgtable) / sizeof(msgtable[0]); i++)
    {	char *id = msgtable[i].ident;
	char *p = msgtable[i].name;

	if (!id)
	    id = p;
#if __GNUC__
	fprintf(fp,"static struct { int l; dchar s[%d] ; } LSTRING_%s = { %d,  %s\"%s\" };\n", 
		strlen(p)+1, id, strlen(p), L, p);
#else
	fprintf(fp,"Lstring LSTRING_%s = { %d, %s\"%s\" };\n", id, strlen(p), L, p);
#endif
    }

    for (i = 0; i < sizeof(msgtable) / sizeof(msgtable[0]); i++)
    {	char *id = msgtable[i].ident;
	char *p = msgtable[i].name;

	if (!id)
	    id = p;
#if __GNUC__
	fprintf(fp,"d_string TEXT_%s = (d_string)&LSTRING_%s;\n", id, id);
#else
	fprintf(fp,"d_string TEXT_%s = &LSTRING_%s;\n", id, id);
#endif
    }

    fprintf(fp, "//\n");
    fprintf(fp, "// *** ERROR MESSAGES ***\n");
    fprintf(fp, "//\n");
    fprintf(fp, "char * errmsgtbl[] = {\n");
    for (i = 0; i < sizeof(errtable) / sizeof(errtable[0]); i++)
    {	char *id = errtable[i].ident;
	char *p = errtable[i].name;

	if (!id)
	    id = p;
	fprintf(fp,"\t\"%s\",\n", p);
    }
    fprintf(fp, "};\n");

    fprintf(fp, "unsigned short errcodtbl[] = {\n");
    for (i = 0; i < sizeof(errtable) / sizeof(errtable[0]); i++)
    {
	unsigned v = errtable[i].value;
	fprintf(fp,"\t%uu,\n", v);
    }
    fprintf(fp, "};\n");

    fclose(fp);

    return EXIT_SUCCESS;
}
