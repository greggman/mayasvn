/*=======================================================================*
 |   file name : dbgprint.h
 |-----------------------------------------------------------------------*
 |   function  : debug printing funcitons
 |-----------------------------------------------------------------------*
 |   author    : gregg tavares
 *=======================================================================*/

#ifndef DBGPRINT_H
#define DBGPRINT_H
/**************************** i n c l u d e s ****************************/

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

/*************************** c o n s t a n t s ***************************/


/******************************* t y p e s *******************************/


/***************************** g l o b a l s *****************************/


/****************************** m a c r o s ******************************/


/************************** p r o t o t y p e s **************************/

extern void dbgReset();
extern int dbgSetDebug (int on);
extern int dbgGetNumErrors ();
extern int dbgGetNumWarnings ();
extern int errPrintf (const char *fmt, ...);
extern int warnPrintf (const char *fmt, ...);
extern int statPrintf (const char *fmt, ...);
extern int dbgPrintf (const char *fmt, ...);

#endif /* DBGPRINT_H */





