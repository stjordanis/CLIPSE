   /*******************************************************/
   /*      "C" Language Integrated Production System      */
   /*                                                     */
   /*            CLIPS Version 6.40  01/06/16             */
   /*                                                     */
   /*            INCREMENTAL RESET HEADER FILE            */
   /*******************************************************/

/*************************************************************/
/* Purpose: Provides functionality for the incremental       */
/*   reset of the pattern and join networks when a new       */
/*   rule is added.                                          */
/*                                                           */
/* Principal Programmer(s):                                  */
/*      Gary D. Riley                                        */
/*                                                           */
/* Contributing Programmer(s):                               */
/*                                                           */
/* Revision History:                                         */
/*                                                           */
/*      6.23: Correction for FalseSymbol/TrueSymbol. DR0859  */
/*                                                           */
/*      6.24: Removed INCREMENTAL_RESET compilation flag.    */
/*                                                           */
/*            Renamed BOOLEAN macro type to intBool.         */
/*                                                           */
/*      6.30: Added support for hashed alpha memories and    */
/*            other join network changes.                    */
/*                                                           */
/*            Removed conditional code for unsupported       */
/*            compilers/operating systems (IBM_MCW and       */
/*            MAC_MCW).                                      */
/*                                                           */
/*            Modified EnvSetIncrementalReset to check for   */
/*            the existance of rules.                        */
/*                                                           */
/*            Converted API macros to function calls.        */
/*                                                           */
/*************************************************************/

#ifndef _H_incrrset

#define _H_incrrset

#ifndef _H_ruledef
#include "ruledef.h"
#endif

   void                           IncrementalReset(void *,struct defrule *);
   bool                           EnvGetIncrementalReset(void *);
   int                            EnvSetIncrementalReset(void *,bool);
   bool                           GetIncrementalResetCommand(void *);
   bool                           SetIncrementalResetCommand(void *);

#endif /* _H_incrrset */









