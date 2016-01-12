   /*******************************************************/
   /*      "C" Language Integrated Production System      */
   /*                                                     */
   /*            CLIPS Version 6.40  01/06/16             */
   /*                                                     */
   /*                 DEFFUNCTION MODULE                  */
   /*******************************************************/

/*************************************************************/
/* Purpose:                                                  */
/*                                                           */
/* Principal Programmer(s):                                  */
/*      Brian L. Dantes                                      */
/*                                                           */
/* Contributing Programmer(s):                               */
/*      Gary D. Riley                                        */
/*                                                           */
/* Revision History:                                         */
/*                                                           */
/*      6.23: Correction for FalseSymbol/TrueSymbol. DR0859  */
/*                                                           */
/*            Corrected compilation errors for files         */
/*            generated by constructs-to-c. DR0861           */
/*                                                           */
/*            Changed name of variable log to logName        */
/*            because of Unix compiler warnings of shadowed  */
/*            definitions.                                   */
/*                                                           */
/*      6.24: Renamed BOOLEAN macro type to intBool.         */
/*                                                           */
/*            Corrected code to remove run-time program      */
/*            compiler warning.                              */
/*                                                           */
/*      6.30: Removed conditional code for unsupported       */
/*            compilers/operating systems (IBM_MCW,          */
/*            MAC_MCW, and IBM_TBC).                         */
/*                                                           */
/*            Changed integer type/precision.                */
/*                                                           */
/*            Added missing initializer for ENTITY_RECORD.   */
/*                                                           */
/*            Added const qualifiers to remove C++           */
/*            deprecation warnings.                          */
/*                                                           */
/*            Converted API macros to function calls.        */
/*                                                           */
/*            Changed find construct functionality so that   */
/*            imported modules are search when locating a    */
/*            named construct.                               */
/*                                                           */
/*            Added code to keep track of pointers to        */
/*            constructs that are contained externally to    */
/*            to constructs, DanglingConstructs.             */
/*                                                           */
/*************************************************************/

/* =========================================
   *****************************************
               EXTERNAL DEFINITIONS
   =========================================
   ***************************************** */
#include "setup.h"

#if DEFFUNCTION_CONSTRUCT

#if (BLOAD || BLOAD_ONLY || BLOAD_AND_BSAVE)
#include "bload.h"
#include "dffnxbin.h"
#endif

#if CONSTRUCT_COMPILER && (! RUN_TIME)
#include "dffnxcmp.h"
#endif

#if (! BLOAD_ONLY) && (! RUN_TIME)
#include "constrct.h"
#include "cstrcpsr.h"
#include "dffnxpsr.h"
#include "modulpsr.h"
#endif

#include "envrnmnt.h"

#if (! RUN_TIME)
#include "extnfunc.h"
#endif

#include "dffnxexe.h"

#if DEBUGGING_FUNCTIONS
#include "watch.h"
#endif

#include "argacces.h"
#include "memalloc.h"
#include "cstrccom.h"
#include "router.h"

#define _DFFNXFUN_SOURCE_
#include "dffnxfun.h"

/* =========================================
   *****************************************
      INTERNALLY VISIBLE FUNCTION HEADERS
   =========================================
   ***************************************** */

static void PrintDeffunctionCall(void *,const char *,void *);
static bool EvaluateDeffunctionCall(void *,void *,DATA_OBJECT *);
static void DecrementDeffunctionBusyCount(void *,void *);
static void IncrementDeffunctionBusyCount(void *,void *);
static void DeallocateDeffunctionData(void *);

#if ! RUN_TIME
static void DestroyDeffunctionAction(void *,struct constructHeader *,void *);
static void *AllocateModule(void *);
static void  ReturnModule(void *,void *);
static bool ClearDeffunctionsReady(void *);
#endif

#if (! BLOAD_ONLY) && (! RUN_TIME)
static bool RemoveAllDeffunctions(void *);
static void DeffunctionDeleteError(void *,const char *);
static void SaveDeffunctionHeaders(void *,void *,const char *);
static void SaveDeffunctionHeader(void *,struct constructHeader *,void *);
static void SaveDeffunctions(void *,void *,const char *);
#endif

#if DEBUGGING_FUNCTIONS
static bool DeffunctionWatchAccess(void *,int,bool,EXPRESSION *);
static bool DeffunctionWatchPrint(void *,const char *,int,EXPRESSION *);
#endif

/* =========================================
   *****************************************
          EXTERNALLY VISIBLE FUNCTIONS
   =========================================
   ***************************************** */

/***************************************************
  NAME         : SetupDeffunctions
  DESCRIPTION  : Initializes parsers and access
                 functions for deffunctions
  INPUTS       : None
  RETURNS      : Nothing useful
  SIDE EFFECTS : Deffunction environment initialized
  NOTES        : None
 ***************************************************/
void SetupDeffunctions(
  void *theEnv)
  {
   ENTITY_RECORD deffunctionEntityRecord =
                     { "PCALL", PCALL,0,0,1,
                       PrintDeffunctionCall,PrintDeffunctionCall,
                       NULL,EvaluateDeffunctionCall,NULL,
                       DecrementDeffunctionBusyCount,IncrementDeffunctionBusyCount,
                       NULL,NULL,NULL,NULL,NULL };

   AllocateEnvironmentData(theEnv,DEFFUNCTION_DATA,sizeof(struct deffunctionData),DeallocateDeffunctionData);
   memcpy(&DeffunctionData(theEnv)->DeffunctionEntityRecord,&deffunctionEntityRecord,sizeof(struct entityRecord));   

   InstallPrimitive(theEnv,&DeffunctionData(theEnv)->DeffunctionEntityRecord,PCALL);

   DeffunctionData(theEnv)->DeffunctionModuleIndex =
                RegisterModuleItem(theEnv,"deffunction",
#if (! RUN_TIME)
                                    AllocateModule,ReturnModule,
#else
                                    NULL,NULL,
#endif
#if BLOAD_AND_BSAVE || BLOAD || BLOAD_ONLY
                                    BloadDeffunctionModuleReference,
#else
                                    NULL,
#endif
#if CONSTRUCT_COMPILER && (! RUN_TIME)
                                    DeffunctionCModuleReference,
#else
                                    NULL,
#endif
                                    EnvFindDeffunctionInModule);

   DeffunctionData(theEnv)->DeffunctionConstruct = AddConstruct(theEnv,"deffunction","deffunctions",
#if (! BLOAD_ONLY) && (! RUN_TIME)
                                       ParseDeffunction,
#else
                                       NULL,
#endif
                                       EnvFindDeffunction,
                                       GetConstructNamePointer,GetConstructPPForm,
                                       GetConstructModuleItem,EnvGetNextDeffunction,
                                       SetNextConstruct,EnvIsDeffunctionDeletable,
                                       EnvUndeffunction,
#if (! BLOAD_ONLY) && (! RUN_TIME)
                                       RemoveDeffunction
#else
                                       NULL
#endif
                                       );
#if ! RUN_TIME
   AddClearReadyFunction(theEnv,"deffunction",ClearDeffunctionsReady,0);

#if ! BLOAD_ONLY
#if DEFMODULE_CONSTRUCT
   AddPortConstructItem(theEnv,"deffunction",SYMBOL);
#endif
   AddSaveFunction(theEnv,"deffunction-headers",SaveDeffunctionHeaders,1000);
   AddSaveFunction(theEnv,"deffunctions",SaveDeffunctions,0);
   EnvDefineFunction2(theEnv,"undeffunction",'v',PTIEF UndeffunctionCommand,"UndeffunctionCommand","11w");
#endif

#if DEBUGGING_FUNCTIONS
   EnvDefineFunction2(theEnv,"list-deffunctions",'v',PTIEF ListDeffunctionsCommand,"ListDeffunctionsCommand","01");
   EnvDefineFunction2(theEnv,"ppdeffunction",'v',PTIEF PPDeffunctionCommand,"PPDeffunctionCommand","11w");
#endif

   EnvDefineFunction2(theEnv,"get-deffunction-list",'m',PTIEF GetDeffunctionListFunction,
                   "GetDeffunctionListFunction","01");

   EnvDefineFunction2(theEnv,"deffunction-module",'w',PTIEF GetDeffunctionModuleCommand,
                   "GetDeffunctionModuleCommand","11w");

#if BLOAD_AND_BSAVE || BLOAD || BLOAD_ONLY
   SetupDeffunctionsBload(theEnv);
#endif

#if CONSTRUCT_COMPILER
   SetupDeffunctionCompiler(theEnv);
#endif

#endif

#if DEBUGGING_FUNCTIONS
   AddWatchItem(theEnv,"deffunctions",0,&DeffunctionData(theEnv)->WatchDeffunctions,32,
                DeffunctionWatchAccess,DeffunctionWatchPrint);
#endif

  }
  
/******************************************************/
/* DeallocateDeffunctionData: Deallocates environment */
/*    data for the deffunction construct.             */
/******************************************************/
static void DeallocateDeffunctionData(
  void *theEnv)
  {
#if ! RUN_TIME
   struct deffunctionModule *theModuleItem;
   void *theModule;

#if BLOAD || BLOAD_AND_BSAVE
   if (Bloaded(theEnv)) return;
#endif

   DoForAllConstructs(theEnv,DestroyDeffunctionAction,DeffunctionData(theEnv)->DeffunctionModuleIndex,false,NULL);

   for (theModule = EnvGetNextDefmodule(theEnv,NULL);
        theModule != NULL;
        theModule = EnvGetNextDefmodule(theEnv,theModule))
     {
      theModuleItem = (struct deffunctionModule *)
                      GetModuleItem(theEnv,(struct defmodule *) theModule,
                                    DeffunctionData(theEnv)->DeffunctionModuleIndex);
      rtn_struct(theEnv,deffunctionModule,theModuleItem);
     }
#else
#if MAC_XCD
#pragma unused(theEnv)
#endif
#endif
  }
  
#if ! RUN_TIME
/*****************************************************/
/* DestroyDeffunctionAction: Action used to remove   */
/*   deffunctions as a result of DestroyEnvironment. */
/*****************************************************/
static void DestroyDeffunctionAction(
  void *theEnv,
  struct constructHeader *theConstruct,
  void *buffer)
  {
#if MAC_XCD
#pragma unused(buffer)
#endif
#if (! BLOAD_ONLY) && (! RUN_TIME)
   struct deffunctionStruct *theDeffunction = (struct deffunctionStruct *) theConstruct;
   
   if (theDeffunction == NULL) return;
   
   ReturnPackedExpression(theEnv,theDeffunction->code);

   DestroyConstructHeader(theEnv,&theDeffunction->header);
   
   rtn_struct(theEnv,deffunctionStruct,theDeffunction);
#else
#if MAC_XCD
#pragma unused(theConstruct,theEnv)
#endif
#endif
  }
#endif

/***************************************************
  NAME         : EnvFindDeffunction
  DESCRIPTION  : Searches for a deffunction
  INPUTS       : The name of the deffunction
                 (possibly including a module name)
  RETURNS      : Pointer to the deffunction if
                 found, otherwise NULL
  SIDE EFFECTS : None
  NOTES        : None
 ***************************************************/
void *EnvFindDeffunction(
  void *theEnv,
  const char *dfnxModuleAndName)
  {
   return(FindNamedConstructInModuleOrImports(theEnv,dfnxModuleAndName,DeffunctionData(theEnv)->DeffunctionConstruct));
  }

/***************************************************
  NAME         : EnvFindDeffunctionInModule
  DESCRIPTION  : Searches for a deffunction
  INPUTS       : The name of the deffunction
                 (possibly including a module name)
  RETURNS      : Pointer to the deffunction if
                 found, otherwise NULL
  SIDE EFFECTS : None
  NOTES        : None
 ***************************************************/
void *EnvFindDeffunctionInModule(
  void *theEnv,
  const char *dfnxModuleAndName)
  {
   return(FindNamedConstructInModule(theEnv,dfnxModuleAndName,DeffunctionData(theEnv)->DeffunctionConstruct));
  }

/***************************************************
  NAME         : LookupDeffunctionByMdlOrScope
  DESCRIPTION  : Finds a deffunction anywhere (if
                 module is specified) or in current
                 or imported modules
  INPUTS       : The deffunction name
  RETURNS      : The deffunction (NULL if not found)
  SIDE EFFECTS : Error message printed on
                  ambiguous references
  NOTES        : None
 ***************************************************/
DEFFUNCTION *LookupDeffunctionByMdlOrScope(
  void *theEnv,
  const char *deffunctionName)
  {
   return((DEFFUNCTION *) LookupConstruct(theEnv,DeffunctionData(theEnv)->DeffunctionConstruct,deffunctionName,true));
  }

/***************************************************
  NAME         : LookupDeffunctionInScope
  DESCRIPTION  : Finds a deffunction in current or
                   imported modules (module
                   specifier is not allowed)
  INPUTS       : The deffunction name
  RETURNS      : The deffunction (NULL if not found)
  SIDE EFFECTS : Error message printed on
                  ambiguous references
  NOTES        : None
 ***************************************************/
DEFFUNCTION *LookupDeffunctionInScope(
  void *theEnv,
  const char *deffunctionName)
  {
   return((DEFFUNCTION *) LookupConstruct(theEnv,DeffunctionData(theEnv)->DeffunctionConstruct,deffunctionName,false));
  }

/***************************************************
  NAME         : EnvUndeffunction
  DESCRIPTION  : External interface routine for
                 removing a deffunction
  INPUTS       : Deffunction pointer
  RETURNS      : false if unsuccessful,
                 true otherwise
  SIDE EFFECTS : Deffunction deleted, if possible
  NOTES        : None
 ***************************************************/
bool EnvUndeffunction(
  void *theEnv,
  void *vptr)
  {
#if BLOAD_ONLY || RUN_TIME
   return(false);
#else

#if BLOAD || BLOAD_AND_BSAVE

   if (Bloaded(theEnv) == true)
     return(false);
#endif
   if (vptr == NULL)
      return(RemoveAllDeffunctions(theEnv));
   if (EnvIsDeffunctionDeletable(theEnv,vptr) == false)
     return(false);
   RemoveConstructFromModule(theEnv,(struct constructHeader *) vptr);
   RemoveDeffunction(theEnv,vptr);
   return(true);
#endif
  }

/****************************************************
  NAME         : EnvGetNextDeffunction
  DESCRIPTION  : Accesses list of deffunctions
  INPUTS       : Deffunction pointer
  RETURNS      : The next deffunction, or the
                 first deffunction (if input is NULL)
  SIDE EFFECTS : None
  NOTES        : None
 ****************************************************/
void *EnvGetNextDeffunction(
  void *theEnv,
  void *ptr)
  {
   return((void *) GetNextConstructItem(theEnv,(struct constructHeader *) ptr,DeffunctionData(theEnv)->DeffunctionModuleIndex));
  }

/***************************************************
  NAME         : EnvIsDeffunctionDeletable
  DESCRIPTION  : Determines if a deffunction is
                 executing or referenced by another
                 expression
  INPUTS       : Deffunction pointer
  RETURNS      : true if the deffunction can
                 be deleted, false otherwise
  SIDE EFFECTS : None
  NOTES        : None
 ***************************************************/
bool EnvIsDeffunctionDeletable(
  void *theEnv,
  void *ptr)
  {
   DEFFUNCTION *dptr;

   if (! ConstructsDeletable(theEnv))
     { return false; }

   dptr = (DEFFUNCTION *) ptr;

   return(((dptr->busy == 0) && (dptr->executing == 0)) ? true : false);
  }

#if (! BLOAD_ONLY) && (! RUN_TIME)

/***************************************************
  NAME         : RemoveDeffunction
  DESCRIPTION  : Removes a deffunction
  INPUTS       : Deffunction pointer
  RETURNS      : Nothing useful
  SIDE EFFECTS : Deffunction deallocated
  NOTES        : Assumes deffunction is not in use!!
 ***************************************************/
void RemoveDeffunction(
  void *theEnv,
  void *vdptr)
  {
   DEFFUNCTION *dptr = (DEFFUNCTION *) vdptr;

   if (dptr == NULL)
     return;
   DecrementSymbolCount(theEnv,EnvGetDeffunctionNamePointer(theEnv,(void *) dptr));
   ExpressionDeinstall(theEnv,dptr->code);
   ReturnPackedExpression(theEnv,dptr->code);
   EnvSetDeffunctionPPForm(theEnv,(void *) dptr,NULL);
   ClearUserDataList(theEnv,dptr->header.usrData);
   rtn_struct(theEnv,deffunctionStruct,dptr);
  }

#endif

/********************************************************
  NAME         : UndeffunctionCommand
  DESCRIPTION  : Deletes the named deffunction(s)
  INPUTS       : None
  RETURNS      : Nothing useful
  SIDE EFFECTS : Deffunction(s) removed
  NOTES        : H/L Syntax: (undeffunction <name> | *)
 ********************************************************/
void UndeffunctionCommand(
  void *theEnv)
  {
   UndefconstructCommand(theEnv,"undeffunction",DeffunctionData(theEnv)->DeffunctionConstruct);
  }

/****************************************************************
  NAME         : GetDeffunctionModuleCommand
  DESCRIPTION  : Determines to which module a deffunction belongs
  INPUTS       : None
  RETURNS      : The symbolic name of the module
  SIDE EFFECTS : None
  NOTES        : H/L Syntax: (deffunction-module <dfnx-name>)
 ****************************************************************/
void *GetDeffunctionModuleCommand(
  void *theEnv)
  {
   return(GetConstructModuleCommand(theEnv,"deffunction-module",DeffunctionData(theEnv)->DeffunctionConstruct));
  }

#if DEBUGGING_FUNCTIONS

/****************************************************
  NAME         : PPDeffunctionCommand
  DESCRIPTION  : Displays the pretty-print form of a
                 deffunction
  INPUTS       : None
  RETURNS      : Nothing useful
  SIDE EFFECTS : Pretty-print form displayed to
                 WDISPLAY logical name
  NOTES        : H/L Syntax: (ppdeffunction <name>)
 ****************************************************/
void PPDeffunctionCommand(
  void *theEnv)
  {
   PPConstructCommand(theEnv,"ppdeffunction",DeffunctionData(theEnv)->DeffunctionConstruct);
  }

/***************************************************
  NAME         : ListDeffunctionsCommand
  DESCRIPTION  : Displays all deffunction names
  INPUTS       : None
  RETURNS      : Nothing useful
  SIDE EFFECTS : Deffunction name sprinted
  NOTES        : H/L Interface
 ***************************************************/
void ListDeffunctionsCommand(
  void *theEnv)
  {
   ListConstructCommand(theEnv,"list-deffunctions",DeffunctionData(theEnv)->DeffunctionConstruct);
  }

/***************************************************
  NAME         : EnvListDeffunctions
  DESCRIPTION  : Displays all deffunction names
  INPUTS       : 1) The logical name of the output
                 2) The module
  RETURNS      : Nothing useful
  SIDE EFFECTS : Deffunction name sprinted
  NOTES        : C Interface
 ***************************************************/
void EnvListDeffunctions(
  void *theEnv,
  const char *logicalName,
  struct defmodule *theModule)
  {
   ListConstruct(theEnv,DeffunctionData(theEnv)->DeffunctionConstruct,logicalName,theModule);
  }

#endif

/***************************************************************
  NAME         : GetDeffunctionListFunction
  DESCRIPTION  : Groups all deffunction names into
                 a multifield list
  INPUTS       : A data object buffer to hold
                 the multifield result
  RETURNS      : Nothing useful
  SIDE EFFECTS : Multifield allocated and filled
  NOTES        : H/L Syntax: (get-deffunction-list [<module>])
 ***************************************************************/
void GetDeffunctionListFunction(
  void *theEnv,
  DATA_OBJECT *returnValue)
  {
   GetConstructListFunction(theEnv,"get-deffunction-list",returnValue,DeffunctionData(theEnv)->DeffunctionConstruct);
  }

/***************************************************************
  NAME         : EnvGetDeffunctionList
  DESCRIPTION  : Groups all deffunction names into
                 a multifield list
  INPUTS       : 1) A data object buffer to hold
                    the multifield result
                 2) The module from which to obtain deffunctions
  RETURNS      : Nothing useful
  SIDE EFFECTS : Multifield allocated and filled
  NOTES        : External C access
 ***************************************************************/
void EnvGetDeffunctionList(
  void *theEnv,
  DATA_OBJECT *returnValue,
  struct defmodule *theModule)
  {
   GetConstructList(theEnv,returnValue,DeffunctionData(theEnv)->DeffunctionConstruct,theModule);
  }

/*******************************************************
  NAME         : CheckDeffunctionCall
  DESCRIPTION  : Checks the number of arguments
                 passed to a deffunction
  INPUTS       : 1) Deffunction pointer
                 2) The number of arguments
  RETURNS      : true if OK, false otherwise
  SIDE EFFECTS : Message printed on errors
  NOTES        : None
 *******************************************************/
bool CheckDeffunctionCall(
  void *theEnv,
  void *vdptr,
  int args)
  {
   DEFFUNCTION *dptr;

   if (vdptr == NULL)
     return(false);
   dptr = (DEFFUNCTION *) vdptr;
   if (args < dptr->minNumberOfParameters)
     {
      if (dptr->maxNumberOfParameters == -1)
        ExpectedCountError(theEnv,EnvGetDeffunctionName(theEnv,(void *) dptr),
                           AT_LEAST,dptr->minNumberOfParameters);
      else
        ExpectedCountError(theEnv,EnvGetDeffunctionName(theEnv,(void *) dptr),
                           EXACTLY,dptr->minNumberOfParameters);
      return(false);
     }
   else if ((args > dptr->minNumberOfParameters) &&
            (dptr->maxNumberOfParameters != -1))
     {
      ExpectedCountError(theEnv,EnvGetDeffunctionName(theEnv,(void *) dptr),
                         EXACTLY,dptr->minNumberOfParameters);
      return(false);
     }
   return(true);
  }

/* =========================================
   *****************************************
          INTERNALLY VISIBLE FUNCTIONS
   =========================================
   ***************************************** */

/***************************************************
  NAME         : PrintDeffunctionCall
  DESCRIPTION  : PrintExpression() support function
                 for deffunction calls
  INPUTS       : 1) The output logical name
                 2) The deffunction
  RETURNS      : Nothing useful
  SIDE EFFECTS : Call expression printed
  NOTES        : None
 ***************************************************/
static void PrintDeffunctionCall(
  void *theEnv,
  const char *logName,
  void *value)
  {
#if DEVELOPER

   EnvPrintRouter(theEnv,logName,"(");
   EnvPrintRouter(theEnv,logName,EnvGetDeffunctionName(theEnv,value));
   if (GetFirstArgument() != NULL)
     {
      EnvPrintRouter(theEnv,logName," ");
      PrintExpression(theEnv,logName,GetFirstArgument());
     }
   EnvPrintRouter(theEnv,logName,")");
#else
#if MAC_XCD
#pragma unused(theEnv)
#pragma unused(logName)
#pragma unused(value)
#endif
#endif
  }

/*******************************************************
  NAME         : EvaluateDeffunctionCall
  DESCRIPTION  : Primitive support function for
                 calling a deffunction
  INPUTS       : 1) The deffunction
                 2) A data object buffer to hold
                    the evaluation result
  RETURNS      : false if the deffunction
                 returns the symbol FALSE,
                 true otherwise
  SIDE EFFECTS : Data obejct buffer set and any
                 side-effects of calling the deffunction
  NOTES        : None
 *******************************************************/
static bool EvaluateDeffunctionCall(
  void *theEnv,
  void *value,
  DATA_OBJECT *result)
  {
   CallDeffunction(theEnv,(DEFFUNCTION *) value,GetFirstArgument(),result);
   if ((GetpType(result) == SYMBOL) &&
       (GetpValue(result) == EnvFalseSymbol(theEnv)))
     return(false);
   return(true);
  }

/***************************************************
  NAME         : DecrementDeffunctionBusyCount
  DESCRIPTION  : Lowers the busy count of a
                 deffunction construct
  INPUTS       : The deffunction
  RETURNS      : Nothing useful
  SIDE EFFECTS : Busy count decremented if a clear
                 is not in progress (see comment)
  NOTES        : None
 ***************************************************/
static void DecrementDeffunctionBusyCount(
  void *theEnv,
  void *value)
  {
   /* ==============================================
      The deffunctions to which expressions in other
      constructs may refer may already have been
      deleted - thus, it is important not to modify
      the busy flag during a clear.
      ============================================== */
   if (! ConstructData(theEnv)->ClearInProgress)
     ((DEFFUNCTION *) value)->busy--;
  }

/***************************************************
  NAME         : IncrementDeffunctionBusyCount
  DESCRIPTION  : Raises the busy count of a
                 deffunction construct
  INPUTS       : The deffunction
  RETURNS      : Nothing useful
  SIDE EFFECTS : Busy count incremented
  NOTES        : None
 ***************************************************/
static void IncrementDeffunctionBusyCount(
  void *theEnv,
  void *value)
  {
#if MAC_XCD
#pragma unused(theEnv)
#endif
#if (! RUN_TIME) && (! BLOAD_ONLY)
   if (! ConstructData(theEnv)->ParsingConstruct)
     { ConstructData(theEnv)->DanglingConstructs++; }
#endif

   ((DEFFUNCTION *) value)->busy++;
  }

#if ! RUN_TIME

/*****************************************************
  NAME         : AllocateModule
  DESCRIPTION  : Creates and initializes a
                 list of deffunctions for a new module
  INPUTS       : None
  RETURNS      : The new deffunction module
  SIDE EFFECTS : Deffunction module created
  NOTES        : None
 *****************************************************/
static void *AllocateModule(
  void *theEnv)
  {
   return((void *) get_struct(theEnv,deffunctionModule));
  }

/***************************************************
  NAME         : ReturnModule
  DESCRIPTION  : Removes a deffunction module and
                 all associated deffunctions
  INPUTS       : The deffunction module
  RETURNS      : Nothing useful
  SIDE EFFECTS : Module and deffunctions deleted
  NOTES        : None
 ***************************************************/
static void ReturnModule(
  void *theEnv,
  void *theItem)
  {
#if (! BLOAD_ONLY)
   FreeConstructHeaderModule(theEnv,(struct defmoduleItemHeader *) theItem,DeffunctionData(theEnv)->DeffunctionConstruct);
#endif
   rtn_struct(theEnv,deffunctionModule,theItem);
  }

/***************************************************
  NAME         : ClearDeffunctionsReady
  DESCRIPTION  : Determines if it is safe to
                 remove all deffunctions
                 Assumes *all* constructs will be
                 deleted - only checks to see if
                 any deffunctions are currently
                 executing
  INPUTS       : None
  RETURNS      : true if no deffunctions are
                 executing, false otherwise
  SIDE EFFECTS : None
  NOTES        : Used by (clear) and (bload)
 ***************************************************/
static bool ClearDeffunctionsReady(
  void *theEnv)
  {
   return((DeffunctionData(theEnv)->ExecutingDeffunction != NULL) ? false : true);
  }

#endif

#if (! BLOAD_ONLY) && (! RUN_TIME)

/***************************************************
  NAME         : RemoveAllDeffunctions
  DESCRIPTION  : Removes all deffunctions
  INPUTS       : None
  RETURNS      : true if all deffunctions
                 removed, false otherwise
  SIDE EFFECTS : Deffunctions removed
  NOTES        : None
 ***************************************************/
static bool RemoveAllDeffunctions(
  void *theEnv)
  {
   DEFFUNCTION *dptr,*dtmp;
   unsigned oldbusy;
   bool success = true;

#if BLOAD || BLOAD_AND_BSAVE

   if (Bloaded(theEnv) == true)
     return(false);
#endif

   dptr = (DEFFUNCTION *) EnvGetNextDeffunction(theEnv,NULL);
   while (dptr != NULL)
     {
      if (dptr->executing > 0)
        {
         DeffunctionDeleteError(theEnv,EnvGetDeffunctionName(theEnv,(void *) dptr));
         success = false;
        }
      else
        {
         oldbusy = dptr->busy;
         ExpressionDeinstall(theEnv,dptr->code);
         dptr->busy = oldbusy;
         ReturnPackedExpression(theEnv,dptr->code);
         dptr->code = NULL;
        }
      dptr = (DEFFUNCTION *) EnvGetNextDeffunction(theEnv,(void *) dptr);
     }

   dptr = (DEFFUNCTION *) EnvGetNextDeffunction(theEnv,NULL);
   while (dptr != NULL)
     {
      dtmp = dptr;
      dptr = (DEFFUNCTION *) EnvGetNextDeffunction(theEnv,(void *) dptr);
      if (dtmp->executing == 0)
        {
         if (dtmp->busy > 0)
           {
            PrintWarningID(theEnv,"DFFNXFUN",1,false);
            EnvPrintRouter(theEnv,WWARNING,"Deffunction ");
            EnvPrintRouter(theEnv,WWARNING,EnvGetDeffunctionName(theEnv,(void *) dtmp));
            EnvPrintRouter(theEnv,WWARNING," only partially deleted due to usage by other constructs.\n");
            EnvSetDeffunctionPPForm(theEnv,(void *) dtmp,NULL);
            success = false;
           }
         else
           {
            RemoveConstructFromModule(theEnv,(struct constructHeader *) dtmp);
            RemoveDeffunction(theEnv,dtmp);
           }
        }
     }
   return(success);
  }

/****************************************************
  NAME         : DeffunctionDeleteError
  DESCRIPTION  : Prints out an error message when
                 a deffunction deletion attempt fails
  INPUTS       : The deffunction name
  RETURNS      : Nothing useful
  SIDE EFFECTS : Error message printed
  NOTES        : None
 ****************************************************/
static void DeffunctionDeleteError(
  void *theEnv,
  const char *dfnxName)
  {
   CantDeleteItemErrorMessage(theEnv,"deffunction",dfnxName);
  }

/***************************************************
  NAME         : SaveDeffunctionHeaders
  DESCRIPTION  : Writes out deffunction forward
                 declarations for (save) command
  INPUTS       : The logical output name
  RETURNS      : Nothing useful
  SIDE EFFECTS : Writes out deffunctions with no
                 body of actions
  NOTES        : Used for deffunctions which are
                 mutually recursive with other
                 constructs
 ***************************************************/
static void SaveDeffunctionHeaders(
  void *theEnv,
  void *theModule,
  const char *logicalName)
  {
   DoForAllConstructsInModule(theEnv,theModule,SaveDeffunctionHeader,
                              DeffunctionData(theEnv)->DeffunctionModuleIndex,
                              false,(void *) logicalName);
  }

/***************************************************
  NAME         : SaveDeffunctionHeader
  DESCRIPTION  : Writes a deffunction forward
                 declaration to the save file
  INPUTS       : 1) The deffunction
                 2) The logical name of the output
  RETURNS      : Nothing useful
  SIDE EFFECTS : Defffunction header written
  NOTES        : None
 ***************************************************/
static void SaveDeffunctionHeader(
  void *theEnv,
  struct constructHeader *theDeffunction,
  void *userBuffer)
  {
   DEFFUNCTION *dfnxPtr = (DEFFUNCTION *) theDeffunction;
   const char *logicalName = (const char *) userBuffer;
   register int i;

   if (EnvGetDeffunctionPPForm(theEnv,(void *) dfnxPtr) != NULL)
     {
      EnvPrintRouter(theEnv,logicalName,"(deffunction ");
      EnvPrintRouter(theEnv,logicalName,EnvDeffunctionModule(theEnv,(void *) dfnxPtr));
      EnvPrintRouter(theEnv,logicalName,"::");      
      EnvPrintRouter(theEnv,logicalName,EnvGetDeffunctionName(theEnv,(void *) dfnxPtr));
      EnvPrintRouter(theEnv,logicalName," (");
      for (i = 0 ; i < dfnxPtr->minNumberOfParameters ; i++)
        {
         EnvPrintRouter(theEnv,logicalName,"?p");
         PrintLongInteger(theEnv,logicalName,(long long) i);
         if (i != dfnxPtr->minNumberOfParameters-1)
           EnvPrintRouter(theEnv,logicalName," ");
        }
      if (dfnxPtr->maxNumberOfParameters == -1)
        {
         if (dfnxPtr->minNumberOfParameters != 0)
           EnvPrintRouter(theEnv,logicalName," ");
         EnvPrintRouter(theEnv,logicalName,"$?wildargs))\n\n");
        }
      else
        EnvPrintRouter(theEnv,logicalName,"))\n\n");
     }
  }

/***************************************************
  NAME         : SaveDeffunctions
  DESCRIPTION  : Writes out deffunctions
                 for (save) command
  INPUTS       : The logical output name
  RETURNS      : Nothing useful
  SIDE EFFECTS : Writes out deffunctions
  NOTES        : None
 ***************************************************/
static void SaveDeffunctions(
  void *theEnv,
  void *theModule,
  const char *logicalName)
  {
   SaveConstruct(theEnv,theModule,logicalName,DeffunctionData(theEnv)->DeffunctionConstruct);
  }

#endif

#if DEBUGGING_FUNCTIONS

/******************************************************************
  NAME         : DeffunctionWatchAccess
  DESCRIPTION  : Parses a list of deffunction names passed by
                 AddWatchItem() and sets the traces accordingly
  INPUTS       : 1) A code indicating which trace flag is to be set
                    Ignored
                 2) The value to which to set the trace flags
                 3) A list of expressions containing the names
                    of the deffunctions for which to set traces
  RETURNS      : true if all OK, false otherwise
  SIDE EFFECTS : Watch flags set in specified deffunctions
  NOTES        : Accessory function for AddWatchItem()
 ******************************************************************/
static bool DeffunctionWatchAccess(
  void *theEnv,
  int code,
  bool newState,
  EXPRESSION *argExprs)
  {
#if MAC_XCD
#pragma unused(code)
#endif

   return(ConstructSetWatchAccess(theEnv,DeffunctionData(theEnv)->DeffunctionConstruct,newState,argExprs,
                                    EnvGetDeffunctionWatch,EnvSetDeffunctionWatch));
  }

/***********************************************************************
  NAME         : DeffunctionWatchPrint
  DESCRIPTION  : Parses a list of deffunction names passed by
                 AddWatchItem() and displays the traces accordingly
  INPUTS       : 1) The logical name of the output
                 2) A code indicating which trace flag is to be examined
                    Ignored
                 3) A list of expressions containing the names
                    of the deffunctions for which to examine traces
  RETURNS      : true if all OK, false otherwise
  SIDE EFFECTS : Watch flags displayed for specified deffunctions
  NOTES        : Accessory function for AddWatchItem()
 ***********************************************************************/
static bool DeffunctionWatchPrint(
  void *theEnv,
  const char *logName,
  int code,
  EXPRESSION *argExprs)
  {
#if MAC_XCD
#pragma unused(code)
#endif

   return(ConstructPrintWatchAccess(theEnv,DeffunctionData(theEnv)->DeffunctionConstruct,logName,argExprs,
                                    EnvGetDeffunctionWatch,EnvSetDeffunctionWatch));
  }

/*********************************************************
  NAME         : EnvSetDeffunctionWatch
  DESCRIPTION  : Sets the trace to ON/OFF for the
                 deffunction
  INPUTS       : 1) true to set the trace on,
                    false to set it off
                 2) A pointer to the deffunction
  RETURNS      : Nothing useful
  SIDE EFFECTS : Watch flag for the deffunction set
  NOTES        : None
 *********************************************************/
void EnvSetDeffunctionWatch(
  void *theEnv,
  bool newState,
  void *dptr)
  {
#if MAC_XCD
#pragma unused(theEnv)
#endif

   ((DEFFUNCTION *) dptr)->trace = newState;
  }

/*********************************************************
  NAME         : EnvGetDeffunctionWatch
  DESCRIPTION  : Determines if trace messages are
                 gnerated when executing deffunction
  INPUTS       : A pointer to the deffunction
  RETURNS      : true if a trace is active,
                 false otherwise
  SIDE EFFECTS : None
  NOTES        : None
 *********************************************************/
bool EnvGetDeffunctionWatch(
  void *theEnv,
  void *dptr)
  {
#if MAC_XCD
#pragma unused(theEnv)
#endif

   return(((DEFFUNCTION *) dptr)->trace);
  }

#endif

/*##################################*/
/* Additional Environment Functions */
/*##################################*/

const char *EnvDeffunctionModule(
  void *theEnv,
  void *theDeffunction)
  {
   return GetConstructModuleName((struct constructHeader *) theDeffunction);
  }

const char *EnvGetDeffunctionName(
  void *theEnv,
  void *theDeffunction)
  {
   return GetConstructNameString((struct constructHeader *) theDeffunction);
  }

const char *EnvGetDeffunctionPPForm(
  void *theEnv,
  void *theDeffunction)
  {
   return GetConstructPPForm(theEnv,(struct constructHeader *) theDeffunction);
  }

SYMBOL_HN *EnvGetDeffunctionNamePointer(
  void *theEnv,
  void *theDeffunction)
  {
   return GetConstructNamePointer((struct constructHeader *) theDeffunction);
  }

void EnvSetDeffunctionPPForm(
  void *theEnv,
  void *theDeffunction,
  const char *thePPForm)
  {
   SetConstructPPForm(theEnv,(struct constructHeader *) theDeffunction,thePPForm);
  }

#endif


