/* ====================================================================
 * Copyright (c) 1999-2001 Carnegie Mellon University.  All rights
 * reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer. 
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * This work was supported in part by funding from the Defense Advanced 
 * Research Projects Agency and the National Science Foundation of the 
 * United States of America, and the CMU Sphinx Speech Consortium.
 *
 * THIS SOFTWARE IS PROVIDED BY CARNEGIE MELLON UNIVERSITY ``AS IS'' AND 
 * ANY EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY
 * NOR ITS EMPLOYEES BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * ====================================================================
 *
 */
/* PCONF.C
 * HISTORY
 * 07-Jan-90  Jeff Rosenfeld (jdr) at Carnegie-Mellon University
 *	Fixed env_scan() so it no int32er writes into it's string arg.
 *	This is to prevent run-time errors when string constants are
 *	stored in non-writable space.
 *
 * 22-Aug-89  Jeff Rosenfeld (jdr) at Carnegie-Mellon University
 *	Added env_scan(), included decls.h for declaration of salloc().
 *
 * 24-May-89  Jeff Rosenfeld (jdr) at Carnegie-Mellon University
 *	Removed definitions of unused variables.
 *
 * 30-Jun-88  Eric Thayer (eht) at Carnegie-Mellon University
 *	Added configuration from file function fpconf().
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>

#include "s2types.h"
#include "c.h"
#include "pconf.h"
#include "strfuncs.h"

static int SetVal(Config_t *cp, char const *str);
static void SPrintVal(Config_t *cp, char *str);


/* PCONF
 *------------------------------------------------------------*
 */
int
pconf (int argc, char *argv[], config_t *config_p,
       char **display, char **geometry,
       char * (*GetDefault)(char const *, char const *))
{
    int32 i, parsed;
    int32 bad_usage = FALSE;
    char *str_p;
    Config_t *cp;

    if (GetDefault) {
	for (cp = (Config_t *) config_p; cp->arg_type != NOTYPE; cp++) {
	    if ((str_p = GetDefault (argv[0], cp->LongName))) {
		bad_usage |= SetVal (cp, str_p);
	    }
	}
    }

    for (i = 1; i < argc; i++) {
	for (parsed = FALSE, cp = (Config_t *) config_p;
	     cp->arg_type != NOTYPE; cp++) {
	    /* FIXME: do we *really* need our own strcasecmp? */
	    if (mystrcasecmp (argv[i], cp->swtch) == 0) {
		parsed = TRUE;
		if (++i < argc)
		    bad_usage |= SetVal (cp, argv[i]);
		else
		    bad_usage = TRUE;
	    }
	}

	if (parsed)
	    continue;
	if (geometry && (*argv[i] == '=')) {
	    *geometry = argv[i];
	    continue;
	}
	if (display && (strchr (argv[i], ':') != 0)) {
	    *display = argv[i];
	    continue;
	}
	if ((mystrcasecmp ("-?", argv[i]) == 0) ||
	    (mystrcasecmp ("-help", argv[i]) == 0))
	    pusage (argv[0], (Config_t *) config_p);
	bad_usage = TRUE;
    }
    return (bad_usage);
}

/* PPCONF
 *------------------------------------------------------------*
 */
int
ppconf (int argc, char *argv[], config_t *config_p,
	char **display, char **geometry,
	char * (*GetDefault)(char const *, char const *),
	char last)
{
    int32 i, parsed;
    int32 bad_usage = FALSE;
    char *str_p;
    Config_t *cp;

    if (GetDefault) {
	for (cp = (Config_t *)config_p; cp->arg_type != NOTYPE; cp++) {
		if ((str_p = GetDefault (argv[0], cp->LongName))) {
		    bad_usage |= SetVal (cp, str_p);
	    }
	}
    }

    for (i = 1; i < argc; i++) {
	/* argument has been processed already */
	if (argv[i][0] == '\0') continue;

	for (parsed = FALSE, cp = (Config_t *)config_p;
	     cp->arg_type != NOTYPE; cp++) {
	    if (mystrcasecmp (argv[i], cp->swtch) == 0) {
		parsed = TRUE;
		/* remove this switch from consideration */
		argv[i][0] = '\0';
		if (++i < argc) {
		    bad_usage |= SetVal (cp, argv[i]);
		    argv[i][0] = '\0';
		}
		else bad_usage = TRUE;
	    }
	}

	if (parsed || !last) continue;

	if (geometry && (*argv[i] == '=')) {
	    *geometry = argv[i];
	    continue;
	}

	if (display && (strchr (argv[i], ':') != 0)) {
	    *display = argv[i];
	    continue;
	}

	if ((mystrcasecmp ("-?", argv[i]) == 0) ||
	    (mystrcasecmp ("-help", argv[i]) == 0))
	    pusage (argv[0], (Config_t *) config_p);
	printf ("%s: Unrecognized argument, %s\n", argv[0], argv[i]);
	bad_usage = TRUE;
    }
    return (bad_usage);
}

/* PUSAGE
 *------------------------------------------------------------*
 */
void
pusage (char *prog, Config_t *cp)
{
    char valstr[256];

    printf ("Usage: %s\n", prog);
    for (; cp->arg_type != NOTYPE; cp++) {
	SPrintVal (cp, valstr);
	printf (" [%s %s] %s - %s\n", cp->swtch, valstr,
		cp->LongName, cp->Doc);
    }
    exit (-1);
}

/* env_scan: substitutes environment values into a string */
static char *
env_scan(char const *str)
{
    extern char *getenv();
    char buf[1024];		/* buffer for temp use */
    register char *p = buf;	/* holds place in the buffer */
    char var[50];		/* holds the name of the env variable */
    char *val;

    while (*str)
	if (*str == '$') {
	    if (*++str == '$') {
		*p++ = *str++;
		continue;
	    }
	    val = var;
	    while (isalnum((unsigned char) *str) || *str == '_')
		*val++ = *str++;
	    *p = '\0';
	    val = (val == var) ? "$" : getenv(var);
	    if (val) {
		strcat(p, val);
		p += strlen(val);
	    }
	} else
	    *p++ = *str++;
    *p = '\0';
    return salloc(buf);
}

static int
SetVal (Config_t *cp, char const *str)
{
    switch (cp->arg_type) {
    case CHAR:
	*(cp->var.CharP) = (char) str[0];
	break;
    case BYTE:
	*(cp->var.ByteP) = (char) atoi (str);
	break;
    case U_BYTE:
	*(cp->var.UByteP) = (u_char) atoi (str);
	break;
    case SHORT:
	*(cp->var.ShortP) = (int16) atoi (str);
	break;
    case U_SHORT:
	*(cp->var.UShortP) = (uint16) atoi (str);
	break;
    case INT:
	*(cp->var.IntP) = (int32) atoi (str);
	break;
    case U_INT:
	*(cp->var.UIntP) = (u_int) atoi (str);
	break;
    case FLOAT:
	*(cp->var.FloatP) = (float) atof (str);
	break;
    case DOUBLE:
	*(cp->var.DoubleP) = (double) atof (str);
	break;
    case BOOL:
	if ((mystrcasecmp ("on", str) == 0) ||
	    (mystrcasecmp ("1", str) == 0) ||
	    (mystrcasecmp ("t", str) == 0) ||
	    (mystrcasecmp ("true", str) == 0) ||
	    (mystrcasecmp ("y", str) == 0) ||
	    (mystrcasecmp ("yes", str) == 0))
	    *(cp->var.BoolP) = TRUE;
	else
	if ((mystrcasecmp ("off", str) == 0) ||
	    (mystrcasecmp ("0", str) == 0) ||
	    (mystrcasecmp ("f", str) == 0) ||
	    (mystrcasecmp ("false", str) == 0) ||
	    (mystrcasecmp ("n", str) == 0) ||
	    (mystrcasecmp ("no", str) == 0))
	    *(cp->var.BoolP) = FALSE;
	else
	    return (-1);
	break;
    case STRING:
	*(cp->var.StringP) = env_scan(str);
	break;
    case DATA_SRC:
	if (mystrcasecmp("hsa", str) == 0)
	    *(cp->var.DataSrcP) = SRC_HSA;
	else if (mystrcasecmp("vqfile", str) == 0)
	    *(cp->var.DataSrcP) = SRC_VQFILE;
	else if (mystrcasecmp("adcfile", str) == 0)
	    *(cp->var.DataSrcP) = SRC_ADCFILE;
	else {
	    printf ("Unknown data source %s\n", str);
	    exit (-1);
	}
	break;
    default:
	fprintf (stderr, "bad param type %d\n", cp->arg_type);
	return (-1);
    }
    return (0);
}

/* FIXME: potential buffer overruns? */
static void
SPrintVal (Config_t *cp, char *str)
{
    switch (cp->arg_type) {
    case CHAR:
	sprintf (str, "%c", *(cp->var.CharP));
	break;
    case BYTE:
	sprintf (str, "%d", *(cp->var.ByteP));
	break;
    case U_BYTE:
	sprintf (str, "%u", *(cp->var.UByteP));
	break;
    case SHORT:
	sprintf (str, "%d", *(cp->var.ShortP));
	break;
    case U_SHORT:
	sprintf (str, "%u", *(cp->var.UShortP));
	break;
    case INT:
	sprintf (str, "%d", *(cp->var.IntP));
	break;
    case U_INT:
	sprintf (str, "%u", *(cp->var.UIntP));
	break;
    case FLOAT:
	sprintf (str, "%f", *(cp->var.FloatP));
	break;
    case DOUBLE:
	sprintf (str, "%f", *(cp->var.DoubleP));
	break;
    case BOOL:
	sprintf (str, "%s", (*(cp->var.BoolP) ? "TRUE" : "FALSE"));
	break;
    case STRING:
        /* Some sprintfs don't like null %s arguments (!) */
	if (*cp->var.StringP)
	    sprintf (str, "%s", *(cp->var.StringP));
	else
	    sprintf (str, "(null)");
	break;
    default:
	sprintf (str, "bad param type %d\n", cp->arg_type);
    }
}

#define MAX_NAME_LEN 32
#define MAX_VALUE_LEN 80

#define NAME 0
#define VALUE 1
#define COMMENT 2

/*
 * fpconf
 */
int
fpconf (FILE *config_fp, Config_t config_p[],
	char **display, char **geometry,
	char * (*GetDefault)(char const *, char const *))
{
    int parsed, read_mode = NAME, inchar;
    int bad_usage = FALSE;
    Config_t *cp;
    char name[MAX_NAME_LEN+1], value[MAX_VALUE_LEN+1], name_fm[12],
	value_fm[12];

    sprintf(name_fm, "%%%d[^:]", MAX_NAME_LEN);
    sprintf(value_fm, "%%%d[^\n]", MAX_VALUE_LEN);

    for (;;) {
	inchar = fgetc(config_fp);

	if (inchar == EOF) {
	    if (read_mode == VALUE)
		fprintf(stderr, "Value expected after name, %s.\n", name);

	    return bad_usage;
	}

	if (inchar == '\n') {
	    read_mode = NAME;
	    continue;
        }

	/* skip whitespace or comments */
        if ( read_mode == COMMENT || inchar == ' ' || inchar == '\t')
	    continue;

	if ( inchar == ';') {
	    read_mode = COMMENT;
	    continue;
	}

	/* put first char of name or value back */
	ungetc(inchar, config_fp);

	if (read_mode == NAME) {
	    fscanf(config_fp, name_fm, name);

	    if (fgetc(config_fp) != ':') {
		fprintf(stderr, "fpconf: Parameter name too int32.");
		exit(-1);
	    }

	    read_mode = VALUE;
	    continue;
	}

	if (read_mode == VALUE) {
	    fscanf(config_fp, value_fm, value);
	    read_mode = NAME;
	}

	/* got a (name, value) pair */

	for (parsed = FALSE, cp = config_p; cp->arg_type != NOTYPE; cp++) {
	    if (mystrcasecmp (name, cp->LongName) == 0) {
		parsed = TRUE;
		bad_usage |= SetVal (cp, salloc(value));

	    }
	}

	if (!parsed) {
	    fprintf(stderr, "fpconf: Unknown parameter %s\n", name);
	    bad_usage = TRUE;
	}
    }
}

