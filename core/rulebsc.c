   /*******************************************************/
   /*      "C" Language Integrated Production System      */
   /*                                                     */
   /*            CLIPS Version 6.40  08/25/16             */
   /*                                                     */
   /*          DEFRULE BASIC COMMANDS HEADER FILE         */
   /*******************************************************/

/*************************************************************/
/* Purpose: Implements core commands for the defrule         */
/*   construct such as clear, reset, save, undefrule,        */
/*   ppdefrule, list-defrules, and                           */
/*   get-defrule-list.                                       */
/*                                                           */
/* Principal Programmer(s):                                  */
/*      Gary D. Riley                                        */
/*                                                           */
/* Contributing Programmer(s):                               */
/*      Brian L. Dantes                                      */
/*                                                           */
/* Revision History:                                         */
/*                                                           */
/*      6.23: Corrected compilation errors for files         */
/*            generated by constructs-to-c. DR0861           */
/*                                                           */
/*            Changed name of variable log to logName        */
/*            because of Unix compiler warnings of shadowed  */
/*            definitions.                                   */
/*                                                           */
/*      6.24: Renamed BOOLEAN macro type to intBool.         */
/*                                                           */
/*      6.30: Removed conditional code for unsupported       */
/*            compilers/operating systems (IBM_MCW,          */
/*            MAC_MCW, and IBM_TBC).                         */
/*                                                           */
/*            Support for join network changes.              */
/*                                                           */
/*            Added const qualifiers to remove C++           */
/*            deprecation warnings.                          */
/*                                                           */
/*            Converted API macros to function calls.        */
/*                                                           */
/*            Added code to prevent a clear command from     */
/*            being executed during fact assertions via      */
/*            JoinOperationInProgress mechanism.             */
/*                                                           */
/*      6.40: Pragma once and other inclusion changes.       */
/*                                                           */
/*            Added support for booleans with <stdbool.h>.   */
/*                                                           */
/*            Removed use of void pointers for specific      */
/*            data structures.                               */
/*                                                           */
/*            ALLOW_ENVIRONMENT_GLOBALS no longer supported. */
/*                                                           */
/*            UDF redesign.                                  */
/*                                                           */
/*************************************************************/

#include "setup.h"

#if DEFRULE_CONSTRUCT

#include <stdio.h>

#include "argacces.h"
#include "constrct.h"
#include "drive.h"
#include "engine.h"
#include "envrnmnt.h"
#include "extnfunc.h"
#include "reteutil.h"
#include "router.h"
#include "ruledef.h"
#include "watch.h"

#if BLOAD || BLOAD_ONLY || BLOAD_AND_BSAVE
#include "rulebin.h"
#endif
#if CONSTRUCT_COMPILER && (! RUN_TIME)
#include "rulecmp.h"
#endif

#include "rulebsc.h"

/***************************************/
/* LOCAL INTERNAL FUNCTION DEFINITIONS */
/***************************************/

   static void                    ResetDefrules(Environment *);
   static void                    ResetDefrulesPrime(Environment *);
   static void                    SaveDefrules(Environment *,Defmodule *,const char *);
#if (! RUN_TIME)
   static bool                    ClearDefrulesReady(Environment *);
   static void                    ClearDefrules(Environment *);
#endif

/*************************************************************/
/* DefruleBasicCommands: Initializes basic defrule commands. */
/*************************************************************/
void DefruleBasicCommands(
  Environment *theEnv)
  {
   EnvAddResetFunction(theEnv,"defrule",ResetDefrules,70);
   EnvAddResetFunction(theEnv,"defrule",ResetDefrulesPrime,10);
   AddSaveFunction(theEnv,"defrule",SaveDefrules,0);
#if (! RUN_TIME)
   AddClearReadyFunction(theEnv,"defrule",ClearDefrulesReady,0);
   EnvAddClearFunction(theEnv,"defrule",ClearDefrules,0);
#endif

#if DEBUGGING_FUNCTIONS
   AddWatchItem(theEnv,"rules",0,&DefruleData(theEnv)->WatchRules,70,DefruleWatchAccess,DefruleWatchPrint);
#endif

#if ! RUN_TIME
   EnvAddUDF(theEnv,"get-defrule-list","m",0,1,"y",GetDefruleListFunction,"GetDefruleListFunction",NULL);
   EnvAddUDF(theEnv,"undefrule","v",1,1,"y",UndefruleCommand,"UndefruleCommand",NULL);
   EnvAddUDF(theEnv,"defrule-module","y",1,1,"y",DefruleModuleFunction,"DefruleModuleFunction",NULL);

#if DEBUGGING_FUNCTIONS
   EnvAddUDF(theEnv,"rules","v",0,1,"y",ListDefrulesCommand,"ListDefrulesCommand",NULL);
   EnvAddUDF(theEnv,"list-defrules","v",0,1,"y",ListDefrulesCommand,"ListDefrulesCommand",NULL);
   EnvAddUDF(theEnv,"ppdefrule","v",1,1,"y",PPDefruleCommand,"PPDefruleCommand",NULL);
#endif

#if (BLOAD || BLOAD_ONLY || BLOAD_AND_BSAVE)
   DefruleBinarySetup(theEnv);
#endif

#if CONSTRUCT_COMPILER && (! RUN_TIME)
   DefruleCompilerSetup(theEnv);
#endif

#endif
  }

/*****************************************************/
/* ResetDefrules: Defrule reset routine for use with */
/*   the reset command. Sets the current entity time */
/*   tag (used by the conflict resolution strategies */
/*   for recency) to zero. The focus stack is also   */
/*   cleared.                                        */
/*****************************************************/
static void ResetDefrules(
  Environment *theEnv)
  {
   Defmodule *theModule;
   struct joinLink *theLink;
   struct partialMatch *notParent;

   DefruleData(theEnv)->CurrentEntityTimeTag = 1L;
   EnvClearFocusStack(theEnv);
   theModule = EnvFindDefmodule(theEnv,"MAIN");
   EnvFocus(theEnv,theModule);

   for (theLink = DefruleData(theEnv)->RightPrimeJoins;
        theLink != NULL;
        theLink = theLink->next)
     { PosEntryRetractAlpha(theEnv,theLink->join->rightMemory->beta[0],NETWORK_ASSERT); }

   for (theLink = DefruleData(theEnv)->LeftPrimeJoins;
        theLink != NULL;
        theLink = theLink->next)
     {
      if ((theLink->join->patternIsNegated || theLink->join->joinFromTheRight) &&
          (! theLink->join->patternIsExists))
        {
         notParent = theLink->join->leftMemory->beta[0];

         if (notParent->marker)
           { RemoveBlockedLink(notParent); }

         /*==========================================================*/
         /* Prevent any retractions from generating partial matches. */
         /*==========================================================*/

         notParent->marker = notParent;

         if (notParent->children != NULL)
           { PosEntryRetractBeta(theEnv,notParent,notParent->children,NETWORK_ASSERT); }
           /*
         if (notParent->dependents != NULL)
           { RemoveLogicalSupport(theEnv,notParent); } */
        }
     }
  }

/***********************/
/* ResetDefrulesPrime: */
/***********************/
static void ResetDefrulesPrime(
  Environment *theEnv)
  {
   struct joinLink *theLink;
   struct partialMatch *notParent;

   for (theLink = DefruleData(theEnv)->RightPrimeJoins;
        theLink != NULL;
        theLink = theLink->next)
     { NetworkAssert(theEnv,theLink->join->rightMemory->beta[0],theLink->join); }

   for (theLink = DefruleData(theEnv)->LeftPrimeJoins;
        theLink != NULL;
        theLink = theLink->next)
     {
      if ((theLink->join->patternIsNegated || theLink->join->joinFromTheRight) &&
          (! theLink->join->patternIsExists))
        {
         notParent = theLink->join->leftMemory->beta[0];

         if (theLink->join->secondaryNetworkTest != NULL)
           {
            if (EvaluateSecondaryNetworkTest(theEnv,notParent,theLink->join) == false)
              { continue; }
           }

         notParent->marker = NULL;

         EPMDrive(theEnv,notParent,theLink->join,NETWORK_ASSERT);
        }
     }

  }

#if (! RUN_TIME)

/******************************************************************/
/* ClearDefrulesReady: Indicates whether defrules can be cleared. */
/******************************************************************/
static bool ClearDefrulesReady(
  Environment *theEnv)
  {
   if (EngineData(theEnv)->ExecutingRule != NULL) return false;

   if (EngineData(theEnv)->JoinOperationInProgress) return false;

   EnvClearFocusStack(theEnv);
   if (EnvGetCurrentModule(theEnv) == NULL) return false;

   DefruleData(theEnv)->CurrentEntityTimeTag = 1L;

   return true;
  }

/***************************************************************/
/* ClearDefrules: Pushes the MAIN module as the current focus. */
/***************************************************************/
static void ClearDefrules(
  Environment *theEnv)
  {
   Defmodule *theModule;

   theModule = EnvFindDefmodule(theEnv,"MAIN");
   EnvFocus(theEnv,theModule);
  }

#endif

/**************************************/
/* SaveDefrules: Defrule save routine */
/*   for use with the save command.   */
/**************************************/
static void SaveDefrules(
  Environment *theEnv,
  Defmodule *theModule,
  const char *logicalName)
  {
   SaveConstruct(theEnv,theModule,logicalName,DefruleData(theEnv)->DefruleConstruct);
  }

/******************************************/
/* UndefruleCommand: H/L access routine   */
/*   for the undefrule command.           */
/******************************************/
void UndefruleCommand(
  Environment *theEnv,
  UDFContext *context,
  CLIPSValue *returnValue)
  {
   UndefconstructCommand(context,"undefrule",DefruleData(theEnv)->DefruleConstruct);
  }

/**********************************/
/* EnvUndefrule: C access routine */
/*   for the undefrule command.   */
/**********************************/
bool EnvUndefrule(
  Environment *theEnv,
  Defrule *theDefrule)
  {
   return(Undefconstruct(theEnv,theDefrule,DefruleData(theEnv)->DefruleConstruct));
  }

/************************************************/
/* GetDefruleListFunction: H/L access routine   */
/*   for the get-defrule-list function.         */
/************************************************/
void GetDefruleListFunction(
  Environment *theEnv,
  UDFContext *context,
  CLIPSValue *returnValue)
  {
   GetConstructListFunction(context,returnValue,DefruleData(theEnv)->DefruleConstruct);
  }

/****************************************/
/* EnvGetDefruleList: C access routine  */
/*   for the get-defrule-list function. */
/****************************************/
void EnvGetDefruleList(
  Environment *theEnv,
  CLIPSValue *returnValue,
  Defmodule *theModule)
  {
   GetConstructList(theEnv,returnValue,DefruleData(theEnv)->DefruleConstruct,theModule);
  }

/*********************************************/
/* DefruleModuleFunction: H/L access routine */
/*   for the defrule-module function.        */
/*********************************************/
void DefruleModuleFunction(
  Environment *theEnv,
  UDFContext *context,
  CLIPSValue *returnValue)
  {
   returnValue->value = GetConstructModuleCommand(context,"defrule-module",DefruleData(theEnv)->DefruleConstruct);
  }

#if DEBUGGING_FUNCTIONS

/******************************************/
/* PPDefruleCommand: H/L access routine   */
/*   for the ppdefrule command.           */
/******************************************/
void PPDefruleCommand(
  Environment *theEnv,
  UDFContext *context,
  CLIPSValue *returnValue)
  {
   PPConstructCommand(context,"ppdefrule",DefruleData(theEnv)->DefruleConstruct);
  }

/***********************************/
/* PPDefrule: C access routine for */
/*   the ppdefrule command.        */
/***********************************/
bool PPDefrule(
  Environment *theEnv,
  const char *defruleName,
  const char *logicalName)
  {
   return(PPConstruct(theEnv,defruleName,logicalName,DefruleData(theEnv)->DefruleConstruct));
  }

/*********************************************/
/* ListDefrulesCommand: H/L access routine   */
/*   for the list-defrules command.          */
/*********************************************/
void ListDefrulesCommand(
  Environment *theEnv,
  UDFContext *context,
  CLIPSValue *returnValue)
  {
   ListConstructCommand(context,DefruleData(theEnv)->DefruleConstruct);
  }

/*************************************/
/* EnvListDefrules: C access routine */
/*   for the list-defrules command.  */
/*************************************/
void EnvListDefrules(
  Environment *theEnv,
  const char *logicalName,
  Defmodule *theModule)
  {
   ListConstruct(theEnv,DefruleData(theEnv)->DefruleConstruct,logicalName,theModule);
  }

/*******************************************************/
/* EnvGetDefruleWatchActivations: C access routine for */
/*   retrieving the current watch value of a defrule's */
/*   activations.                                      */
/*******************************************************/
bool EnvGetDefruleWatchActivations(
  Environment *theEnv,
  Defrule *rulePtr)
  {
   Defrule *thePtr;
#if MAC_XCD
#pragma unused(theEnv)
#endif

   for (thePtr = rulePtr;
        thePtr != NULL;
        thePtr = thePtr->disjunct)
     { if (thePtr->watchActivation) return true; }

   return false;
  }

/***********************************************/
/* EnvGetDefruleWatchFirings: C access routine */
/*   for retrieving the current watch value of */
/*   a defrule's firings.                      */
/***********************************************/
bool EnvGetDefruleWatchFirings(
  Environment *theEnv,
  Defrule *rulePtr)
  {
   Defrule *thePtr;
#if MAC_XCD
#pragma unused(theEnv)
#endif

   for (thePtr = rulePtr;
        thePtr != NULL;
        thePtr = thePtr->disjunct)
     { if (thePtr->watchFiring) return true; }

   return false;
  }

/***************************************************/
/* EnvSetDefruleWatchActivations: C access routine */
/*   for setting the current watch value of a      */
/*   defrule's activations.                        */
/***************************************************/
void EnvSetDefruleWatchActivations(
  Environment *theEnv,
  bool newState,
  Defrule *rulePtr)
  {
   Defrule *thePtr;
#if MAC_XCD
#pragma unused(theEnv)
#endif

   for (thePtr = rulePtr;
        thePtr != NULL;
        thePtr = thePtr->disjunct)
     { thePtr->watchActivation = newState; }
  }

/****************************************************/
/* EnvSetDefruleWatchFirings: C access routine for  */
/*   setting the current watch value of a defrule's */
/*   firings.                                       */
/****************************************************/
void EnvSetDefruleWatchFirings(
  Environment *theEnv,
  bool newState,
  Defrule *rulePtr)
  {
   Defrule *thePtr;
#if MAC_XCD
#pragma unused(theEnv)
#endif

   for (thePtr = rulePtr;
        thePtr != NULL;
        thePtr = thePtr->disjunct)
     { thePtr->watchFiring = newState; }
  }

/*******************************************************************/
/* DefruleWatchAccess: Access function for setting the watch flags */
/*   associated with rules (activations and rule firings).         */
/*******************************************************************/
bool DefruleWatchAccess(
  Environment *theEnv,
  int code,
  bool newState,
  struct expr *argExprs)
  {
   if (code)
     return(ConstructSetWatchAccess(theEnv,DefruleData(theEnv)->DefruleConstruct,newState,argExprs,
                                    (bool (*)(Environment *,void *)) EnvGetDefruleWatchActivations,
                                    (void (*)(Environment *,bool,void *)) EnvSetDefruleWatchActivations));
   else
     return(ConstructSetWatchAccess(theEnv,DefruleData(theEnv)->DefruleConstruct,newState,argExprs,
                                    (bool (*)(Environment *,void *)) EnvGetDefruleWatchFirings,
                                    (void (*)(Environment *,bool,void *)) EnvSetDefruleWatchFirings));
  }

/*****************************************************************/
/* DefruleWatchPrint: Access routine for printing which defrules */
/*   have their watch flag set via the list-watch-items command. */
/*****************************************************************/
bool DefruleWatchPrint(
  Environment *theEnv,
  const char *logName,
  int code,
  struct expr *argExprs)
  {
   if (code)
     return(ConstructPrintWatchAccess(theEnv,DefruleData(theEnv)->DefruleConstruct,logName,argExprs,
                                      (bool (*)(Environment *,void *)) EnvGetDefruleWatchActivations,
                                      (void (*)(Environment *,bool,void *)) EnvSetDefruleWatchActivations));
   else
     return(ConstructPrintWatchAccess(theEnv,DefruleData(theEnv)->DefruleConstruct,logName,argExprs,
                                      (bool (*)(Environment *,void *)) EnvGetDefruleWatchActivations,
                                      (void (*)(Environment *,bool,void *)) EnvSetDefruleWatchActivations));
  }

#endif /* DEBUGGING_FUNCTIONS */

#endif /* DEFTEMPLATE_CONSTRUCT */
