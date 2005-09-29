/*=======================================================================*
 |   file name : dbgprint.cpp
 |-----------------------------------------------------------------------*
 |   function  : debug printing funcitons
 |-----------------------------------------------------------------------*
 |   author    : gregg tavares
 *=======================================================================*/

/**************************** i n c l u d e s ****************************/

#include <maya/mglobal.h>
#include "dbgprint.h"

/*************************** c o n s t a n t s ***************************/

#define DBG_WARN	1
#define DBG_ERROR	2
#define DBG_DEBUG	3
#define DBG_STATUS	4

/******************************* t y p e s *******************************/


/************************** p r o t o t y p e s **************************/


/***************************** g l o b a l s *****************************/

static int numErrors;
static int numWarnings;
static int fDebug;

/****************************** m a c r o s ******************************/


/**************************** r o u t i n e s ****************************/

void dbgReset() { numErrors = 0; numWarnings = 0; fDebug = false; }
int dbgGetNumErrors () { return numErrors; }
int dbgGetNumWarnings () { return numWarnings; }
int dbgSetDebug (int on) { int old = fDebug; fDebug = on; return old; }

static void dbgPrintToAll (const char* str, int type)
{
	OutputDebugString (str);

	switch (type)
	{
	case DBG_WARN:
		MGlobal::displayWarning (str);
		break;
	case DBG_ERROR:
		MGlobal::displayError (str);
		break;
	case DBG_DEBUG:
		printf ("%s", str);
		break;
	case DBG_STATUS:
		MGlobal::displayInfo (str);
		break;
	}
}

/*************************************************************************
                              dbgVPrintf
 *************************************************************************

   SYNOPSIS
		int dbgVPrintf (const char *pszFormat, va_list ap)

   PURPOSE
      Handles routine of printing depending on platform.

   INPUT
      pszFormat :  format string that was passed to va_start().
		ap        :  va_list that has already been started with va_start().

   OUTPUT
		None

   EFFECTS
		None

   RETURNS


   SEE ALSO


   HISTORY
		11/30/95 JMA: Created.

 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

int dbgVPrintf (const char *pszFormat, va_list ap, int type)
{
#define DBG_VPRINTF_CHARS_MAX  1023

	char szTemp[DBG_VPRINTF_CHARS_MAX+1];
	int len;

	len = _vsnprintf (szTemp, DBG_VPRINTF_CHARS_MAX, pszFormat,ap);

	if (len == -1 && len == DBG_VPRINTF_CHARS_MAX)
	{
		strcpy (szTemp, "dbgVPrintf() needs larger string buffer!\n");
	}

	dbgPrintToAll (szTemp, type);

	return len;

#undef DBG_VPRINTF_CHARS_MAX
}


/*************************************************************************
                                 dbgPrintf
 *************************************************************************

   SYNOPSIS
		int dbgPrintf (const char *frmt, ...)

   PURPOSE
      A cross platform no hassles printf routine that we can do with what
      we want.

   INPUT
		frmt :   Format string.
		 ...  :   arguments.

   OUTPUT
		None

   EFFECTS
		None

   RETURNS


   SEE ALSO


   HISTORY
		11/30/95 JMA: Created.

 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

int dbgPrintf (const char *frmt, ...)
{
	if (fDebug)
	{
		int len;

		va_list ap;	/* points to each unnamed arg in turn */

		va_start (ap,frmt); /* make ap point to 1st unnamed arg */
		len = dbgVPrintf (frmt, ap, DBG_DEBUG);
		va_end (ap);	/* clean up when done */

		return len;
	}
	return 0;
}


/*************************************************************************
                                 errPrintf
 *************************************************************************

   SYNOPSIS
		int errPrintf (const char *frmt, ...)

   PURPOSE
      A cross platform no hassles printf routine that we can do with what
      we want.

   INPUT
		frmt :   Format string.
		 ...  :   arguments.

   OUTPUT
		None

   EFFECTS
		None

   RETURNS


   SEE ALSO


   HISTORY
		11/30/95 JMA: Created.

 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

int errVPrintf (const char* fmt, va_list ap)
{
	++numErrors;
	OutputDebugString ("ERROR : ");
	return dbgVPrintf (fmt, ap, DBG_ERROR);
}
int errPrintf (const char *frmt, ...)
{
	int len;
	va_list ap;	/* points to each unnamed arg in turn */

	va_start (ap,frmt); /* make ap point to 1st unnamed arg */
	len = errVPrintf (frmt, ap);
	va_end (ap);	/* clean up when done */

	OutputDebugString ("\n");

	return len;
}

int warnPrintf (const char *frmt, ...)
{
	int len;
	va_list ap;	/* points to each unnamed arg in turn */

	++numWarnings;
	OutputDebugString ("WARNING : ");

	va_start (ap,frmt); /* make ap point to 1st unnamed arg */
	len = dbgVPrintf (frmt, ap, DBG_WARN);
	va_end (ap);	/* clean up when done */

	OutputDebugString ("\n");

	return len;
}

int statPrintf (const char *frmt, ...)
{
	int len;
	va_list ap;	/* points to each unnamed arg in turn */

	va_start (ap,frmt); /* make ap point to 1st unnamed arg */
	len = dbgVPrintf (frmt, ap, DBG_STATUS);
	va_end (ap);	/* clean up when done */

	OutputDebugString ("\n");

	return len;
}



