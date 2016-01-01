   /*******************************************************/
   /*      "C" Language Integrated Production System      */
   /*                                                     */
   /*               CLIPS Version 6.30  01/25/15          */
   /*                                                     */
   /*                                                     */
   /*******************************************************/

/*************************************************************/
/* Purpose:                                                  */
/*                                                           */
/* Principal Programmer(s):                                  */
/*      Brian L. Dantes                                      */
/*                                                           */
/* Contributing Programmer(s):                               */
/*                                                           */
/* Revision History:                                         */
/*      6.23: Corrected compilation errors for files         */
/*            generated by constructs-to-c. DR0861           */
/*                                                           */
/*      6.24: Renamed BOOLEAN macro type to intBool.         */
/*                                                           */
/*      6.30: Removed conditional code for unsupported       */
/*            compilers/operating systems (IBM_MCW,          */
/*            MAC_MCW, and IBM_TBC).                         */
/*                                                           */
/*            Changed integer type/precision.                */
/*                                                           */
/*            Added const qualifiers to remove C++           */
/*            deprecation warnings.                          */
/*                                                           */
/*            Converted API macros to function calls.        */
/*                                                           */
/*            Fixed linkage issue when DEBUGGING_FUNCTIONS   */
/*            is set to 0 and PROFILING_FUNCTIONS is set to  */
/*            1.                                             */
/*                                                           */
/*            Changed find construct functionality so that   */
/*            imported modules are search when locating a    */
/*            named construct.                               */
/*                                                           */
/*************************************************************/

#ifndef _H_genrccom
#define _H_genrccom

#ifndef _H_constrct
#include "constrct.h"
#endif
#ifndef _H_cstrccom
#include "cstrccom.h"
#endif
#ifndef _H_evaluatn
#include "evaluatn.h"
#endif
#ifndef _H_moduldef
#include "moduldef.h"
#endif
#ifndef _H_genrcfun
#include "genrcfun.h"
#endif
#ifndef _H_symbol
#include "symbol.h"
#endif

   void                           SetupGenericFunctions(void *);
   void                          *EnvFindDefgeneric(void *,const char *);
   void                          *EnvFindDefgenericInModule(void *,const char *);
   DEFGENERIC                    *LookupDefgenericByMdlOrScope(void *,const char *);
   DEFGENERIC                    *LookupDefgenericInScope(void *,const char *);
   void                          *EnvGetNextDefgeneric(void *,void *);
   long                           EnvGetNextDefmethod(void *,void *,long);
   int                            EnvIsDefgenericDeletable(void *,void *);
   int                            EnvIsDefmethodDeletable(void *,void *,long);
   void                           UndefgenericCommand(void *);
   void                          *GetDefgenericModuleCommand(void *);
   void                           UndefmethodCommand(void *);
   DEFMETHOD                     *GetDefmethodPointer(void *,long);
   intBool                        EnvUndefgeneric(void *,void *);
   intBool                        EnvUndefmethod(void *,void *,long);
#if ! OBJECT_SYSTEM
   void                           TypeCommand(void *,DATA_OBJECT *);
#endif
#if DEBUGGING_FUNCTIONS || PROFILING_FUNCTIONS
   void                           EnvGetDefmethodDescription(void *,char *,size_t,void *,long);
#endif
#if DEBUGGING_FUNCTIONS
   unsigned                       EnvGetDefgenericWatch(void *,void *);
   void                           EnvSetDefgenericWatch(void *,unsigned,void *);
   unsigned                       EnvGetDefmethodWatch(void *,void *,long);
   void                           EnvSetDefmethodWatch(void *,unsigned,void *,long);
   void                           PPDefgenericCommand(void *);
   void                           PPDefmethodCommand(void *);
   void                           ListDefmethodsCommand(void *);
   const char                    *EnvGetDefmethodPPForm(void *,void *,long);
   void                           ListDefgenericsCommand(void *);
   void                           EnvListDefgenerics(void *,const char *,struct defmodule *);
   void                           EnvListDefmethods(void *,const char *,void *);
#endif
   void                           GetDefgenericListFunction(void *,DATA_OBJECT *);
   void                           EnvGetDefgenericList(void *,DATA_OBJECT *,struct defmodule *);
   void                           GetDefmethodListCommand(void *,DATA_OBJECT *);
   void                           EnvGetDefmethodList(void *,void *,DATA_OBJECT *);
   void                           GetMethodRestrictionsCommand(void *,DATA_OBJECT *);
   void                           EnvGetMethodRestrictions(void *,void *,long,DATA_OBJECT *);
   SYMBOL_HN                     *GetDefgenericNamePointer(void *);
   void                           SetNextDefgeneric(void *,void *);
   const char                    *EnvDefgenericModule(void *,void *);
   const char                    *EnvGetDefgenericName(void *,void *);
   const char                    *EnvGetDefgenericPPForm(void *,void *);
   SYMBOL_HN                     *EnvGetDefgenericNamePointer(void *,void *);
   void                           EnvSetDefgenericPPForm(void *,void *,const char *);

#if ALLOW_ENVIRONMENT_GLOBALS

   void                           SetDefgenericPPForm(void *,const char *);
   const char                    *DefgenericModule(void *);
   void                          *FindDefgeneric(const char *);
   void                           GetDefgenericList(DATA_OBJECT *,struct defmodule *);
   const char                    *GetDefgenericName(void *);
   const char                    *GetDefgenericPPForm(void *);
   void                          *GetNextDefgeneric(void *);
   int                            IsDefgenericDeletable(void *);
   intBool                        Undefgeneric(void *);
   void                           GetDefmethodList(void *,DATA_OBJECT_PTR);
   void                           GetMethodRestrictions(void *,long,DATA_OBJECT *);
   long                           GetNextDefmethod(void *,long );
   int                            IsDefmethodDeletable(void *,long );
   intBool                        Undefmethod(void *,long );
#if DEBUGGING_FUNCTIONS
   unsigned                       GetDefgenericWatch(void *);
   void                           ListDefgenerics(const char *,struct defmodule *);
   void                           SetDefgenericWatch(unsigned,void *);
   const char                    *GetDefmethodPPForm(void *,long);
   unsigned                       GetDefmethodWatch(void *,long);
   void                           ListDefmethods(const char *,void *);
   void                           SetDefmethodWatch(unsigned,void *,long);
#endif /* DEBUGGING_FUNCTIONS */
#if DEBUGGING_FUNCTIONS || PROFILING_FUNCTIONS
   void                           GetDefmethodDescription(char *,int,void *,long );
#endif /* DEBUGGING_FUNCTIONS || PROFILING_FUNCTIONS */

#endif /* ALLOW_ENVIRONMENT_GLOBALS */

#endif /* _H_genrccom */





