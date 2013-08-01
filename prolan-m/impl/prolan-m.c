/* An implementation of the PROLAN/M programming language.

   This software is public domain; anyone obtaining a copy may deal
   in it without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, and/or sell this 
   software.

   The author provides no warranty on this software.

   -- Catatonic Porpoise <graue@oceanbase.org>, October 2005
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Read all the characters up to, but not including, the first
   occurrence of endch. If EOF is read before endch is found, NULL
   is returned. The returned string must be freed by the caller. */
static char *readto(FILE *fp, char endch)
{
	char *str = NULL;
	int strindex = 0;
	int strsize = 0;
	int c;

	strsize = 10;
	str = malloc(strsize + 1);
	if (!str)
	{
		fprintf(stderr, "out of memory\n");
		exit(1);
	}

	while ((c = getc(fp)) != endch)
	{
		if (c == EOF)
		{
			if (str)
				free(str);
			return NULL;
		}

		if (strindex == strsize)
		{
			char *newstr;

			strsize *= 2;
			newstr = realloc(str, strsize + 1);
			if (newstr == NULL)
			{
				fprintf(stderr, "out of memory\n");
				exit(1);
			}
			str = newstr;
		}

		str[strindex] = c;
		strindex++;
	}

	str[strindex] = '\0';

	return str;
}

typedef struct
{
	char *before;
	char *after;
	int beforelen;
	int afterlen;
} rule_t;

/* Read a rule_t or return NULL if there isn't one left in the file.
   "(,)" doesn't count. The caller must free the returned rule_t. */
static rule_t *readrule(FILE *fp)
{
	char *before = NULL;
	char *after  = NULL;
	rule_t *rule = NULL;

	(void) readto(fp, '('); /* devour anything before the next rule */

	before = readto(fp, ',');
	if (before == NULL)
		return NULL;

	after = readto(fp, ')');
	if (after == NULL || strlen(before) == 0)
		return NULL;

	rule = malloc(sizeof (rule_t));
	if (!rule)
	{
		fprintf(stderr, "out of memory\n");
		exit(1);
	}

	rule->before = before;
	rule->after  = after;

	/* store the lengths so we don't have to call strlen() all
	   the time */
	rule->beforelen = strlen(before);
	rule->afterlen  = strlen(after);

	return rule;
}

/* Applies rule rule to ostr, returning a pointer to the modified
   version, possibly at a different memory location. (That is,
   ostr may be freed if necessary.) The caller should eventually
   free the returned pointer.

   NULL is returned if the string is unaffected by the rule. */
static char *applyrule(const rule_t *rule, char *ostr)
{
	char *p   = NULL;
	char *str = NULL;
	int len;

	/* does the rule affect this string? */
	p = strstr(ostr, rule->before);
	if (p == NULL)
		return NULL; /* nope */

	len = strlen(ostr);

	if (rule->beforelen > rule->afterlen)
	{
		/* string must shrink */
		char *dst = NULL;
		char *src = NULL;

		strcpy(p, rule->after);

		src = p + rule->beforelen;
		dst = p + rule->afterlen;

		memmove(dst, src, len - (p - ostr) - rule->beforelen + 1);
		str = ostr;
	}
	else if (rule->beforelen == rule->afterlen)
	{
		/* string stays the same size */
		strncpy(p, rule->after, rule->afterlen);
		str = ostr;
	}
	else
	{
		char *dst = NULL;
		char *src = NULL;

		/* string must enlarge; first give it enough space */
		str = realloc(ostr, len
			+ rule->afterlen - rule->beforelen);
		if (str == NULL)
		{
			fprintf(stderr, "out of memory\n");
			exit(1);
		}

		/* if the memory has moved, update p */
		if (str != ostr)
		{
			ostr = NULL; /* just to be extra safe */
			p = strstr(str, rule->before);
		}

		/* move the old stuff after the substitution down */
		src = p + rule->beforelen;
		dst = p + rule->afterlen;
		memmove(dst, src, len - (p - str) - rule->beforelen + 1);

		/* substitute */
		strncpy(p, rule->after, rule->afterlen);
	}

	return str;
}

/* Apply rules. ostr may be freed (as in applyrule(), which this function
   uses). Returns NULL if no rules apply at all. */
static char *applyrules(rule_t *const *rules, int numrules, char *ostr)
{
	char *retval = NULL;
	int i;

	for (i = 0; i < numrules; i++)
	{
		retval = applyrule(rules[i], ostr);
		if (retval != NULL)
			return retval;
	}

	return NULL;
}

/* Read rules from filename. Set *paprule to a pointer to an array
   of pointers to rules. Return the number of rules read. */
static int readrules(rule_t ***paprule, const char *filename)
{
	rule_t **rules = NULL;
	rule_t  *arule = NULL;
	int numrules = 0;
	int rulespace = 0;
	FILE *fp;

	rulespace = 10;
	rules = malloc(rulespace * sizeof (rule_t *));
	if (!rules)
	{
		fprintf(stderr, "out of memory\n");
		return NULL;
	}

	if (!strcmp(filename, "-"))
		fp = stdin;
	else
	{
		fp = fopen(filename, "r");
		if (!fp)
		{
			fprintf(stderr, "cannot open %s\n", filename);
			exit(1);
		}
	}

	while ((arule = readrule(fp)) != NULL)
	{
		if (numrules == rulespace)
		{
			rule_t **newrules;

			rulespace *= 2;
			newrules = realloc(rules,
				rulespace * sizeof (rule_t *));
			if (newrules == NULL)
			{
				fprintf(stderr, "out of memory\n");
				exit(1);
			}
			rules = newrules;
		}

		rules[numrules] = arule;
		numrules++;
	}

	fclose(fp);

	*paprule = rules;
	return numrules;
}

static void usage(void)
{
	fprintf(stderr, "usage: prolan-m [source file] [input string]\n");
	exit(1);
}

int main(int argc, char *argv[])
{
	rule_t **rules;
	int numrules;
	char *str;

	if (argc != 3)
		usage();

	numrules = readrules(&rules, argv[1]);
	str = strdup(argv[2]);
	if (str == NULL)
	{
		fprintf(stderr, "out of memory\n");
		exit(1);
	}

	/* run the program */
	for (;;)
	{
		char *res;
		res = applyrules(rules, numrules, str);
		if (res == NULL) /* no rules matched */
			break;
		str = res;
	}

	/* print the output */
	printf("%s\n", str);

	return 0;
}
