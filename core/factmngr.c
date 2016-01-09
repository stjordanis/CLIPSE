   /*******************************************************/
   /*      "C" Language Integrated Production System      */
   /*                                                     */
   /*            CLIPS Version 6.40  01/06/16             */
   /*                                                     */
   /*                 FACT MANAGER MODULE                 */
   /*******************************************************/

/*************************************************************/
/* Purpose: Provides core routines for maintaining the fact  */
/*   list including assert/retract operations, data          */
/*   structure creation/deletion, printing, slot access,     */
/*   and other utility functions.                            */
/*                                                           */
/* Principal Programmer(s):                                  */
/*      Gary D. Riley                                        */
/*                                                           */
/* Contributing Programmer(s):                               */
/*      Brian L. Dantes                                      */
/*                                                           */
/* Revision History:                                         */
/*                                                           */
/*      6.23: Added support for templates maintaining their  */
/*            own list of facts.                             */
/*                                                           */
/*      6.24: Removed LOGICAL_DEPENDENCIES compilation flag. */
/*                                                           */
/*            Renamed BOOLEAN macro type to intBool.         */
/*                                                           */
/*            AssignFactSlotDefaults function does not       */
/*            properly handle defaults for multifield slots. */
/*            DR0869                                         */
/*                                                           */
/*            Support for ppfact command.                    */
/*                                                           */
/*      6.30: Callback function support for assertion,       */
/*            retraction, and modification of facts.         */
/*                                                           */
/*            Updates to fact pattern entity record.         */
/*                                                           */
/*            Changed integer type/precision.                */
/*                                                           */
/*            Changed garbage collection algorithm.          */
/*                                                           */
/*            Removed conditional code for unsupported       */
/*            compilers/operating systems (IBM_MCW,          */
/*            MAC_MCW, and IBM_TBC).                         */
/*                                                           */
/*            Added const qualifiers to remove C++           */
/*            deprecation warnings.                          */
/*                                                           */
/*            Converted API macros to function calls.        */
/*                                                           */
/*            Removed unused global variables.               */
/*                                                           */
/*            Added code to prevent a clear command from     */
/*            being executed during fact assertions via      */
/*            JoinOperationInProgress mechanism.             */
/*                                                           */
/*            Added code to keep track of pointers to        */
/*            constructs that are contained externally to    */
/*            to constructs, DanglingConstructs.             */
/*                                                           */
/*      6.40: Added Env prefix to GetEvaluationError and     */
/*            SetEvaluationError functions.                  */
/*                                                           */
/*            Removed initial-fact support.                  */
/*                                                           */
/*            Modify command preserves fact id and address.  */
/*                                                           */
/*            Watch facts for modify command only prints     */
/*            changed slots.                                 */
/*                                                           */
/*************************************************************/


#define _FACTMNGR_SOURCE_

#include <stdio.h>
#define _STDIO_INCLUDED_

#include "setup.h"

#if DEFTEMPLATE_CONSTRUCT && DEFRULE_CONSTRUCT

#include "constant.h"
#include "symbol.h"
#include "memalloc.h"
#include "exprnpsr.h"
#include "argacces.h"
#include "scanner.h"
#include "router.h"
#include "strngrtr.h"
#include "match.h"
#include "factbld.h"
#include "factqury.h"
#include "reteutil.h"
#include "retract.h"
#include "factcmp.h"
#include "filecom.h"
#include "factfun.h"
#include "factcom.h"
#include "constrct.h"
#include "factrhs.h"
#include "factmch.h"
#include "watch.h"
#include "utility.h"
#include "factbin.h"
#include "factmngr.h"
#include "facthsh.h"
#include "default.h"
#include "commline.h"
#include "envrnmnt.h"
#include "sysdep.h"

#include "engine.h"
#include "lgcldpnd.h"
#include "drive.h"
#include "ruledlt.h"

#include "tmpltbsc.h"
#include "tmpltdef.h"
#include "tmpltutl.h"
#include "tmpltfun.h"

/***************************************/
/* LOCAL INTERNAL FUNCTION DEFINITIONS */
/***************************************/

   static void                    ResetFacts(void *);
   static bool                    ClearFactsReady(void *);
   static void                    RemoveGarbageFacts(void *);
   static void                    DeallocateFactData(void *);

/**************************************************************/
/* InitializeFacts: Initializes the fact data representation. */
/*   Facts are only available when both the defrule and       */
/*   deftemplate constructs are available.                    */
/**************************************************************/
void InitializeFacts(
  void *theEnv)
  {
   struct patternEntityRecord factInfo = { { "FACT_ADDRESS", FACT_ADDRESS,1,0,0,
                                                     PrintFactIdentifier,
                                                     PrintFactIdentifierInLongForm,
                                                     EnvRetract,
                                                     NULL,
                                                     EnvGetNextFact,
                                                     EnvDecrementFactCount,
                                                     EnvIncrementFactCount,NULL,NULL,NULL,NULL,NULL
                                                   },
                                                   DecrementFactBasisCount,
                                                   IncrementFactBasisCount,
                                                   MatchFactFunction,
                                                   NULL,
                                                   FactIsDeleted
                                                 };
                                                 
   struct fact dummyFact = { { NULL, NULL, 0, 0L }, NULL, NULL, -1L, 0, 1,
                                  NULL, NULL, NULL, NULL, NULL, { 1, 0UL, NULL, { { 0, NULL } } } };

   AllocateEnvironmentData(theEnv,FACTS_DATA,sizeof(struct factsData),DeallocateFactData);

   memcpy(&FactData(theEnv)->FactInfo,&factInfo,sizeof(struct patternEntityRecord)); 
   dummyFact.factHeader.theInfo = &FactData(theEnv)->FactInfo;    
   memcpy(&FactData(theEnv)->DummyFact,&dummyFact,sizeof(struct fact));  
   FactData(theEnv)->LastModuleIndex = -1;

   /*=========================================*/
   /* Initialize the fact hash table (used to */
   /* quickly determine if a fact exists).    */
   /*=========================================*/

   InitializeFactHashTable(theEnv);

   /*============================================*/
   /* Initialize the fact callback functions for */
   /* use with the reset and clear commands.     */
   /*============================================*/

   EnvAddResetFunction(theEnv,"facts",ResetFacts,60);
   AddClearReadyFunction(theEnv,"facts",ClearFactsReady,0);

   /*=============================*/
   /* Initialize periodic garbage */
   /* collection for facts.       */
   /*=============================*/

   AddCleanupFunction(theEnv,"facts",RemoveGarbageFacts,0);

   /*===================================*/
   /* Initialize fact pattern matching. */
   /*===================================*/

   InitializeFactPatterns(theEnv);

   /*==================================*/
   /* Initialize the facts keyword for */
   /* use with the watch command.      */
   /*==================================*/

#if DEBUGGING_FUNCTIONS
   AddWatchItem(theEnv,"facts",0,&FactData(theEnv)->WatchFacts,80,DeftemplateWatchAccess,DeftemplateWatchPrint);
#endif

   /*=========================================*/
   /* Initialize fact commands and functions. */
   /*=========================================*/

   FactCommandDefinitions(theEnv);
   FactFunctionDefinitions(theEnv);
   
   /*==============================*/
   /* Initialize fact set queries. */
   /*==============================*/
  
#if FACT_SET_QUERIES
   SetupFactQuery(theEnv);
#endif

   /*==================================*/
   /* Initialize fact patterns for use */
   /* with the bload/bsave commands.   */
   /*==================================*/

#if (BLOAD || BLOAD_ONLY || BLOAD_AND_BSAVE) && (! RUN_TIME)
   FactBinarySetup(theEnv);
#endif

   /*===================================*/
   /* Initialize fact patterns for use  */
   /* with the constructs-to-c command. */
   /*===================================*/

#if CONSTRUCT_COMPILER && (! RUN_TIME)
   FactPatternsCompilerSetup(theEnv);
#endif
  }
  
/***********************************/
/* DeallocateFactData: Deallocates */
/*   environment data for facts.   */
/***********************************/
static void DeallocateFactData(
  void *theEnv)
  {
   struct factHashEntry *tmpFHEPtr, *nextFHEPtr;
   struct fact *tmpFactPtr, *nextFactPtr;
   unsigned long i;
   struct patternMatch *theMatch, *tmpMatch;
   
   for (i = 0; i < FactData(theEnv)->FactHashTableSize; i++) 
     {
      tmpFHEPtr = FactData(theEnv)->FactHashTable[i];
      
      while (tmpFHEPtr != NULL)
        {
         nextFHEPtr = tmpFHEPtr->next;
         rtn_struct(theEnv,factHashEntry,tmpFHEPtr);
         tmpFHEPtr = nextFHEPtr;
        }
     }
  
   rm3(theEnv,FactData(theEnv)->FactHashTable,
       sizeof(struct factHashEntry *) * FactData(theEnv)->FactHashTableSize);
                 
   tmpFactPtr = FactData(theEnv)->FactList;
   while (tmpFactPtr != NULL)
     {
      nextFactPtr = tmpFactPtr->nextFact;

      theMatch = (struct patternMatch *) tmpFactPtr->list;
      while (theMatch != NULL)
        {
         tmpMatch = theMatch->next;
         rtn_struct(theEnv,patternMatch,theMatch);
         theMatch = tmpMatch;
        }

      ReturnEntityDependencies(theEnv,(struct patternEntity *) tmpFactPtr);

      ReturnFact(theEnv,tmpFactPtr);
      tmpFactPtr = nextFactPtr; 
     }
     
   tmpFactPtr = FactData(theEnv)->GarbageFacts;
   while (tmpFactPtr != NULL)
     {
      nextFactPtr = tmpFactPtr->nextFact;

      ReturnFact(theEnv,tmpFactPtr);
      tmpFactPtr = nextFactPtr; 
     }
     
   DeallocateCallListWithArg(theEnv,FactData(theEnv)->ListOfAssertFunctions);
   DeallocateCallListWithArg(theEnv,FactData(theEnv)->ListOfRetractFunctions);
   DeallocateCallListWithArg(theEnv,FactData(theEnv)->ListOfModifyFunctions);
  }

/**********************************************/
/* PrintFactWithIdentifier: Displays a single */
/*   fact preceded by its fact identifier.    */
/**********************************************/
void PrintFactWithIdentifier(
  void *theEnv,
  const char *logicalName,
  struct fact *factPtr,
  const char *changeMap)
  {
   char printSpace[20];

   gensprintf(printSpace,"f-%-5lld ",factPtr->factIndex);
   EnvPrintRouter(theEnv,logicalName,printSpace);
   PrintFact(theEnv,logicalName,factPtr,false,false,changeMap);
  }

/****************************************************/
/* PrintFactIdentifier: Displays a fact identifier. */
/****************************************************/
void PrintFactIdentifier(
  void *theEnv,
  const char *logicalName,
  void *factPtr)
  {
   char printSpace[20];

   gensprintf(printSpace,"f-%lld",((struct fact *) factPtr)->factIndex);
   EnvPrintRouter(theEnv,logicalName,printSpace);
  }

/********************************************/
/* PrintFactIdentifierInLongForm: Display a */
/*   fact identifier in a longer format.    */
/********************************************/
void PrintFactIdentifierInLongForm(
  void *theEnv,
  const char *logicalName,
  void *factPtr)
  {
   if (PrintUtilityData(theEnv)->AddressesToStrings) EnvPrintRouter(theEnv,logicalName,"\"");
   if (factPtr != (void *) &FactData(theEnv)->DummyFact)
     {
      EnvPrintRouter(theEnv,logicalName,"<Fact-");
      PrintLongInteger(theEnv,logicalName,((struct fact *) factPtr)->factIndex);
      EnvPrintRouter(theEnv,logicalName,">");
     }
   else
     { EnvPrintRouter(theEnv,logicalName,"<Dummy Fact>"); }

   if (PrintUtilityData(theEnv)->AddressesToStrings) EnvPrintRouter(theEnv,logicalName,"\"");
  }

/*******************************************/
/* DecrementFactBasisCount: Decrements the */
/*   partial match busy count of a fact    */
/*******************************************/
void DecrementFactBasisCount(
  void *theEnv,
  void *vFactPtr)
  {
   struct fact *factPtr = (struct fact *) vFactPtr;
   struct multifield *theSegment;
   int i;

   EnvDecrementFactCount(theEnv,factPtr);

   if (factPtr->basisSlots != NULL)
     {
      theSegment = factPtr->basisSlots;
      factPtr->basisSlots->busyCount--;
     }
   else
     { theSegment = &factPtr->theProposition; }

   for (i = 0 ; i < (int) theSegment->multifieldLength ; i++)
     {
      AtomDeinstall(theEnv,theSegment->theFields[i].type,theSegment->theFields[i].value);
     }

   if ((factPtr->basisSlots != NULL) && (factPtr->basisSlots->busyCount == 0))
     {
      ReturnMultifield(theEnv,factPtr->basisSlots);
      factPtr->basisSlots = NULL;
     }
  }

/*******************************************/
/* IncrementFactBasisCount: Increments the */
/*   partial match busy count of a fact.   */
/*******************************************/
void IncrementFactBasisCount(
  void *theEnv,
  void *vFactPtr)
  {
   struct fact *factPtr = (struct fact *) vFactPtr;
   struct multifield *theSegment;
   int i;

   EnvIncrementFactCount(theEnv,factPtr);

   theSegment = &factPtr->theProposition;

   if (theSegment->multifieldLength != 0)
     {
      if (factPtr->basisSlots != NULL)
        {
         factPtr->basisSlots->busyCount++;
        }
      else
        {
         factPtr->basisSlots = CopyMultifield(theEnv,theSegment);
         factPtr->basisSlots->busyCount = 1;
        }
      theSegment = factPtr->basisSlots;
     }

   for (i = 0 ; i < (int) theSegment->multifieldLength ; i++)
     {
      AtomInstall(theEnv,theSegment->theFields[i].type,theSegment->theFields[i].value);
     }
  }

/******************/
/* FactIsDeleted: */
/******************/
bool FactIsDeleted(
  void *theEnv,
  void *theFact)
  {
#if MAC_XCD
#pragma unused(theEnv)
#endif

   return(((struct fact *) theFact)->garbage);
  }

/**************************************************/
/* PrintFact: Displays the printed representation */
/*   of a fact containing the relation name and   */
/*   all of the fact's slots or fields.           */
/**************************************************/
void PrintFact(
  void *theEnv,
  const char *logicalName,
  struct fact *factPtr,
  bool seperateLines,
  bool ignoreDefaults,
  const char *changeMap)
  {
   struct multifield *theMultifield;

   /*=========================================*/
   /* Print a deftemplate (non-ordered) fact. */
   /*=========================================*/

   if (factPtr->whichDeftemplate->implied == false)
     {
      PrintTemplateFact(theEnv,logicalName,factPtr,seperateLines,ignoreDefaults,changeMap);
      return;
     }

   /*==============================*/
   /* Print an ordered fact (which */
   /* has an implied deftemplate). */
   /*==============================*/

   EnvPrintRouter(theEnv,logicalName,"(");

   EnvPrintRouter(theEnv,logicalName,factPtr->whichDeftemplate->header.name->contents);

   theMultifield = (struct multifield *) factPtr->theProposition.theFields[0].value;
   if (theMultifield->multifieldLength != 0)
     {
      EnvPrintRouter(theEnv,logicalName," ");
      PrintMultifield(theEnv,logicalName,theMultifield,0,
                      (long) (theMultifield->multifieldLength - 1),
                      false);
     }

   EnvPrintRouter(theEnv,logicalName,")");
  }

/*********************************************/
/* MatchFactFunction: Filters a fact through */
/*   the appropriate fact pattern network.   */
/*********************************************/
void MatchFactFunction(
  void *theEnv,
  void *vTheFact)
  {
   struct fact *theFact = (struct fact *) vTheFact;

   FactPatternMatch(theEnv,theFact,theFact->whichDeftemplate->patternNetwork,0,NULL,NULL);
  }

/*************************************************/
/* RetractDriver: Driver routine for EnvRetract. */
/*************************************************/
bool RetractDriver(
  void *theEnv,
  void *vTheFact,
  bool modifyOperation,
  char *changeMap)
  {
   struct fact *theFact = (struct fact *) vTheFact;
   struct deftemplate *theTemplate = theFact->whichDeftemplate;
   struct callFunctionItemWithArg *theRetractFunction;

   /*===========================================*/
   /* A fact can not be retracted while another */
   /* fact is being asserted or retracted.      */
   /*===========================================*/

   if (EngineData(theEnv)->JoinOperationInProgress)
     {
      PrintErrorID(theEnv,"FACTMNGR",1,true);
      EnvPrintRouter(theEnv,WERROR,"Facts may not be retracted during pattern-matching\n");
      return(false);
     }

   /*====================================*/
   /* A NULL fact pointer indicates that */
   /* all facts should be retracted.     */
   /*====================================*/

   if (theFact == NULL)
     {
      RemoveAllFacts(theEnv);
      return(true);
     }

   /*======================================================*/
   /* Check to see if the fact has already been retracted. */
   /*======================================================*/

   if (theFact->garbage) return(false);
   
   /*===========================================*/
   /* Execute the list of functions that are    */
   /* to be called before each fact retraction. */
   /*===========================================*/

   for (theRetractFunction = FactData(theEnv)->ListOfRetractFunctions;
        theRetractFunction != NULL;
        theRetractFunction = theRetractFunction->next)
     {
      SetEnvironmentCallbackContext(theEnv,theRetractFunction->context);
      if (theRetractFunction->environmentAware)
        { (*theRetractFunction->func)(theEnv,theFact); }
      else
        { ((void (*)(void *))(*theRetractFunction->func))(theFact); }
     }

   /*============================*/
   /* Print retraction output if */
   /* facts are being watched.   */
   /*============================*/

#if DEBUGGING_FUNCTIONS
   if (theFact->whichDeftemplate->watch)
     {
      EnvPrintRouter(theEnv,WTRACE,"<== ");
      PrintFactWithIdentifier(theEnv,WTRACE,theFact,changeMap);
      EnvPrintRouter(theEnv,WTRACE,"\n");
     }
#endif

   /*==================================*/
   /* Set the change flag to indicate  */
   /* the fact-list has been modified. */
   /*==================================*/

   FactData(theEnv)->ChangeToFactList = true;

   /*===============================================*/
   /* Remove any links between the fact and partial */
   /* matches in the join network. These links are  */
   /* used to keep track of logical dependencies.   */
   /*===============================================*/

   RemoveEntityDependencies(theEnv,(struct patternEntity *) theFact);

   /*===========================================*/
   /* Remove the fact from the fact hash table. */
   /*===========================================*/

   RemoveHashedFact(theEnv,theFact);

   /*=========================================*/
   /* Remove the fact from its template list. */
   /*=========================================*/

   if (theFact == theTemplate->lastFact)
     { theTemplate->lastFact = theFact->previousTemplateFact; }

   if (theFact->previousTemplateFact == NULL)
     {
      theTemplate->factList = theTemplate->factList->nextTemplateFact;
      if (theTemplate->factList != NULL)
        { theTemplate->factList->previousTemplateFact = NULL; }
     }
   else
     {
      theFact->previousTemplateFact->nextTemplateFact = theFact->nextTemplateFact;
      if (theFact->nextTemplateFact != NULL)
        { theFact->nextTemplateFact->previousTemplateFact = theFact->previousTemplateFact; }
     }

   /*=====================================*/
   /* Remove the fact from the fact list. */
   /*=====================================*/

   if (theFact == FactData(theEnv)->LastFact)
     { FactData(theEnv)->LastFact = theFact->previousFact; }

   if (theFact->previousFact == NULL)
     {
      FactData(theEnv)->FactList = FactData(theEnv)->FactList->nextFact;
      if (FactData(theEnv)->FactList != NULL)
        { FactData(theEnv)->FactList->previousFact = NULL; }
     }
   else
     {
      theFact->previousFact->nextFact = theFact->nextFact;
      if (theFact->nextFact != NULL)
        { theFact->nextFact->previousFact = theFact->previousFact; }
     }

   /*===================================================*/
   /* Add the fact to the fact garbage list unless this */
   /* fact is being retract as part of a modify action. */
   /*===================================================*/

   if (! modifyOperation)
     {
      theFact->nextFact = FactData(theEnv)->GarbageFacts;
      FactData(theEnv)->GarbageFacts = theFact;
      UtilityData(theEnv)->CurrentGarbageFrame->dirty = true;
     }
   else
     {
      theFact->nextFact = NULL;
     }
   theFact->garbage = true;

   /*===================================================*/
   /* Reset the evaluation error flag since expressions */
   /* will be evaluated as part of the retract.         */
   /*===================================================*/

   EnvSetEvaluationError(theEnv,false);

   /*===========================================*/
   /* Loop through the list of all the patterns */
   /* that matched the fact and process the     */
   /* retract operation for each one.           */
   /*===========================================*/

   EngineData(theEnv)->JoinOperationInProgress = true;
   NetworkRetract(theEnv,(struct patternMatch *) theFact->list);
   theFact->list = NULL;
   EngineData(theEnv)->JoinOperationInProgress = false;

   /*=========================================*/
   /* Free partial matches that were released */
   /* by the retraction of the fact.          */
   /*=========================================*/

   if (EngineData(theEnv)->ExecutingRule == NULL)
     { FlushGarbagePartialMatches(theEnv); }

   /*=========================================*/
   /* Retract other facts that were logically */
   /* dependent on the fact just retracted.   */
   /*=========================================*/

   ForceLogicalRetractions(theEnv);

   /*===========================================*/
   /* Force periodic cleanup if the retract was */
   /* executed from an embedded application.    */
   /*===========================================*/

   if ((UtilityData(theEnv)->CurrentGarbageFrame->topLevel) && (! CommandLineData(theEnv)->EvaluatingTopLevelCommand) &&
       (EvaluationData(theEnv)->CurrentExpression == NULL) && (UtilityData(theEnv)->GarbageCollectionLocks == 0))
     { CleanCurrentGarbageFrame(theEnv,NULL); }
   
   /*==================================*/
   /* Update busy counts and ephemeral */
   /* garbage information.             */
   /*==================================*/

   FactDeinstall(theEnv,theFact);

   /*==================================*/
   /* Return true to indicate the fact */
   /* was successfully retracted.      */
   /*==================================*/

   return(true);
  }

/*********************************************************/
/* EnvRetract: C access routine for the retract command. */
/*********************************************************/
bool EnvRetract(
  void *theEnv,
  void *vTheFact)
  {
   return RetractDriver(theEnv,vTheFact,false,NULL);
  }

/*******************************************************************/
/* RemoveGarbageFacts: Returns facts that have been retracted to   */
/*   the pool of available memory. It is necessary to postpone     */
/*   returning the facts to memory because RHS actions retrieve    */
/*   their variable bindings directly from the fact data structure */
/*   and the facts may be in use in other data structures.         */
/*******************************************************************/
static void RemoveGarbageFacts(
  void *theEnv)
  {
   struct fact *factPtr, *nextPtr, *lastPtr = NULL;

   factPtr = FactData(theEnv)->GarbageFacts;

   while (factPtr != NULL)
     {
      nextPtr = factPtr->nextFact;
      if (factPtr->factHeader.busyCount == 0)
        {
         ReturnFact(theEnv,factPtr);
         if (lastPtr == NULL) FactData(theEnv)->GarbageFacts = nextPtr;
         else lastPtr->nextFact = nextPtr;
        }
      else
        { lastPtr = factPtr; }

      factPtr = nextPtr;
     }
  }

/********************************************************/
/* AssertDriver: Driver routine for the assert command. */
/********************************************************/
void *AssertDriver(
  void *theEnv,
  void *vTheFact,
  long long reuseIndex,
  struct fact *factListPosition,
  struct fact *templatePosition,
  char *changeMap)
  {
   unsigned long hashValue;
   unsigned long length, i;
   struct field *theField;
   struct fact *theFact = (struct fact *) vTheFact;
   bool duplicate;
   struct callFunctionItemWithArg *theAssertFunction;

   /*==========================================*/
   /* A fact can not be asserted while another */
   /* fact is being asserted or retracted.     */
   /*==========================================*/

   if (EngineData(theEnv)->JoinOperationInProgress)
     {
      ReturnFact(theEnv,theFact);
      PrintErrorID(theEnv,"FACTMNGR",2,true);
      EnvPrintRouter(theEnv,WERROR,"Facts may not be asserted during pattern-matching\n");
      return(NULL);
     }

   /*=============================================================*/
   /* Replace invalid data types in the fact with the symbol nil. */
   /*=============================================================*/

   length = theFact->theProposition.multifieldLength;
   theField = theFact->theProposition.theFields;

   for (i = 0; i < length; i++)
     {
      if (theField[i].type == RVOID)
        {
         theField[i].type = SYMBOL;
         theField[i].value = (void *) EnvAddSymbol(theEnv,"nil");
        }
     }

   /*========================================================*/
   /* If fact assertions are being checked for duplications, */
   /* then search the fact list for a duplicate fact.        */
   /*========================================================*/

   hashValue = HandleFactDuplication(theEnv,theFact,&duplicate,reuseIndex);
   if (duplicate) return(NULL);

   /*==========================================================*/
   /* If necessary, add logical dependency links between the   */
   /* fact and the partial match which is its logical support. */
   /*==========================================================*/

   if (AddLogicalDependencies(theEnv,(struct patternEntity *) theFact,false) == false)
     {
      if (reuseIndex == 0)
        { ReturnFact(theEnv,theFact); }
      else
        {
         theFact->nextFact = FactData(theEnv)->GarbageFacts;
         FactData(theEnv)->GarbageFacts = theFact;
         UtilityData(theEnv)->CurrentGarbageFrame->dirty = true;
         theFact->garbage = true;
        }
      return(NULL);
     }

   /*======================================*/
   /* Add the fact to the fact hash table. */
   /*======================================*/

   AddHashedFact(theEnv,theFact,hashValue);

   /*================================*/
   /* Add the fact to the fact list. */
   /*================================*/

   if (reuseIndex == 0)
     { factListPosition = FactData(theEnv)->LastFact; }
     
   if (factListPosition == NULL)
     {
      theFact->nextFact = FactData(theEnv)->FactList;
      FactData(theEnv)->FactList = theFact;
      theFact->previousFact = NULL;
      if (theFact->nextFact != NULL)
        { theFact->nextFact->previousFact = theFact; }
     }
   else
     {
      theFact->nextFact = factListPosition->nextFact;
      theFact->previousFact = factListPosition;
      factListPosition->nextFact = theFact;
      if (theFact->nextFact != NULL)
        { theFact->nextFact->previousFact = theFact; }
     }

   if ((FactData(theEnv)->LastFact == NULL) || (theFact->nextFact == NULL))
     { FactData(theEnv)->LastFact = theFact; }

   /*====================================*/
   /* Add the fact to its template list. */
   /*====================================*/

   if (reuseIndex == 0)
     { templatePosition = theFact->whichDeftemplate->lastFact; }
     
   if (templatePosition == NULL)
     {
      theFact->nextTemplateFact = theFact->whichDeftemplate->factList;
      theFact->whichDeftemplate->factList = theFact;
      theFact->previousTemplateFact = NULL;
      if (theFact->nextTemplateFact != NULL)
        { theFact->nextTemplateFact->previousTemplateFact = theFact; }
     }
   else
     {
      theFact->nextTemplateFact = templatePosition->nextTemplateFact;
      theFact->previousTemplateFact = templatePosition;
      templatePosition->nextTemplateFact = theFact;
      if (theFact->nextTemplateFact != NULL)
        { theFact->nextTemplateFact->previousTemplateFact = theFact; }
     }

   if ((theFact->whichDeftemplate->lastFact == NULL) || (theFact->nextTemplateFact == NULL))
     { theFact->whichDeftemplate->lastFact = theFact; }

   /*==================================*/
   /* Set the fact index and time tag. */
   /*==================================*/
   
   if (reuseIndex > 0)
     { theFact->factIndex = reuseIndex; }
   else
     { theFact->factIndex = FactData(theEnv)->NextFactIndex++; }
     
   theFact->factHeader.timeTag = DefruleData(theEnv)->CurrentEntityTimeTag++;

   /*=====================*/
   /* Update busy counts. */
   /*=====================*/

   FactInstall(theEnv,theFact);
   
   /*==========================================*/
   /* Execute the list of functions that are   */
   /* to be called before each fact assertion. */
   /*==========================================*/

   for (theAssertFunction = FactData(theEnv)->ListOfAssertFunctions;
        theAssertFunction != NULL;
        theAssertFunction = theAssertFunction->next)
     {
      SetEnvironmentCallbackContext(theEnv,theAssertFunction->context);
      if (theAssertFunction->environmentAware)
        { (*theAssertFunction->func)(theEnv,theFact); }
      else
        { ((void (*)(void *))(*theAssertFunction->func))(theFact); }
     }

   /*==========================*/
   /* Print assert output if   */
   /* facts are being watched. */
   /*==========================*/

#if DEBUGGING_FUNCTIONS
   if (theFact->whichDeftemplate->watch)
     {
      EnvPrintRouter(theEnv,WTRACE,"==> ");
      PrintFactWithIdentifier(theEnv,WTRACE,theFact,changeMap);
      EnvPrintRouter(theEnv,WTRACE,"\n");
     }
#endif

   /*==================================*/
   /* Set the change flag to indicate  */
   /* the fact-list has been modified. */
   /*==================================*/

   FactData(theEnv)->ChangeToFactList = true;

   /*==========================================*/
   /* Check for constraint errors in the fact. */
   /*==========================================*/

   CheckTemplateFact(theEnv,theFact);

   /*===================================================*/
   /* Reset the evaluation error flag since expressions */
   /* will be evaluated as part of the assert .         */
   /*===================================================*/

   EnvSetEvaluationError(theEnv,false);

   /*=============================================*/
   /* Pattern match the fact using the associated */
   /* deftemplate's pattern network.              */
   /*=============================================*/

   EngineData(theEnv)->JoinOperationInProgress = true;
   FactPatternMatch(theEnv,theFact,theFact->whichDeftemplate->patternNetwork,0,NULL,NULL);
   EngineData(theEnv)->JoinOperationInProgress = false;

   /*===================================================*/
   /* Retract other facts that were logically dependent */
   /* on the non-existence of the fact just asserted.   */
   /*===================================================*/

   ForceLogicalRetractions(theEnv);

   /*=========================================*/
   /* Free partial matches that were released */
   /* by the assertion of the fact.           */
   /*=========================================*/

   if (EngineData(theEnv)->ExecutingRule == NULL) FlushGarbagePartialMatches(theEnv);

   /*==========================================*/
   /* Force periodic cleanup if the assert was */
   /* executed from an embedded application.   */
   /*==========================================*/

   if ((UtilityData(theEnv)->CurrentGarbageFrame->topLevel) && (! CommandLineData(theEnv)->EvaluatingTopLevelCommand) &&
       (EvaluationData(theEnv)->CurrentExpression == NULL) && (UtilityData(theEnv)->GarbageCollectionLocks == 0))
     {
      CleanCurrentGarbageFrame(theEnv,NULL);
      CallPeriodicTasks(theEnv);
     }

   /*===============================*/
   /* Return a pointer to the fact. */
   /*===============================*/

   return((void *) theFact);
  }

/********************************************************/
/* EnvAssert: C access routine for the assert function. */
/********************************************************/
void *EnvAssert(
  void *theEnv,
  void *vTheFact)
  {
   return AssertDriver(theEnv,vTheFact,0,NULL,NULL,NULL);
  }

/**************************************/
/* RemoveAllFacts: Loops through the  */
/*   fact-list and removes each fact. */
/**************************************/
void RemoveAllFacts(
  void *theEnv)
  {
   while (FactData(theEnv)->FactList != NULL)
     { EnvRetract(theEnv,(void *) FactData(theEnv)->FactList); }
  }

/************************************************/
/* EnvCreateFact: Creates a fact data structure */
/*   of the specified deftemplate.              */
/************************************************/
struct fact *EnvCreateFact(
  void *theEnv,
  void *vTheDeftemplate)
  {
   struct deftemplate *theDeftemplate = (struct deftemplate *) vTheDeftemplate;
   struct fact *newFact;
   int i;

   /*=================================*/
   /* A deftemplate must be specified */
   /* in order to create a fact.      */
   /*=================================*/

   if (theDeftemplate == NULL) return(NULL);

   /*============================================*/
   /* Create a fact for an explicit deftemplate. */
   /*============================================*/

   if (theDeftemplate->implied == false)
     {
      newFact = CreateFactBySize(theEnv,theDeftemplate->numberOfSlots);
      for (i = 0;
           i < (int) theDeftemplate->numberOfSlots;
           i++)
        { newFact->theProposition.theFields[i].type = RVOID; }
     }

   /*===========================================*/
   /* Create a fact for an implied deftemplate. */
   /*===========================================*/

   else
     {
      newFact = CreateFactBySize(theEnv,1);
      newFact->theProposition.theFields[0].type = MULTIFIELD;
      newFact->theProposition.theFields[0].value = CreateMultifield2(theEnv,0L);
     }

   /*===============================*/
   /* Return a pointer to the fact. */
   /*===============================*/

   newFact->whichDeftemplate = theDeftemplate;

   return(newFact);
  }

/******************************************/
/* EnvGetFactSlot: Returns the slot value */
/*   from the specified slot of a fact.   */
/******************************************/
bool EnvGetFactSlot(
  void *theEnv,
  void *vTheFact,
  const char *slotName,
  DATA_OBJECT *theValue)
  {
   struct fact *theFact = (struct fact *) vTheFact;
   struct deftemplate *theDeftemplate;
   short whichSlot;

   /*===============================================*/
   /* Get the deftemplate associated with the fact. */
   /*===============================================*/

   theDeftemplate = theFact->whichDeftemplate;

   /*==============================================*/
   /* Handle retrieving the slot value from a fact */
   /* having an implied deftemplate. An implied    */
   /* facts has a single multifield slot.          */
   /*==============================================*/

   if (theDeftemplate->implied)
     {
      if (slotName != NULL) return(false);
      theValue->type = theFact->theProposition.theFields[0].type;
      theValue->value = theFact->theProposition.theFields[0].value;
      SetpDOBegin(theValue,1);
      SetpDOEnd(theValue,((struct multifield *) theValue->value)->multifieldLength);
      return(true);
     }

   /*===================================*/
   /* Make sure the slot name requested */
   /* corresponds to a valid slot name. */
   /*===================================*/

   if (FindSlot(theDeftemplate,(SYMBOL_HN *) EnvAddSymbol(theEnv,slotName),&whichSlot) == NULL)
     { return(false); }

   /*======================================================*/
   /* Return the slot value. If the slot value wasn't set, */
   /* then return false to indicate that an appropriate    */
   /* slot value wasn't available.                         */
   /*======================================================*/

   theValue->type = theFact->theProposition.theFields[whichSlot-1].type;
   theValue->value = theFact->theProposition.theFields[whichSlot-1].value;
   if (theValue->type == MULTIFIELD)
     {
      SetpDOBegin(theValue,1);
      SetpDOEnd(theValue,((struct multifield *) theValue->value)->multifieldLength);
     }

   if (theValue->type == RVOID) return(false);

   return(true);
  }

/***************************************/
/* EnvPutFactSlot: Sets the slot value */
/*   of the specified slot of a fact.  */
/***************************************/
bool EnvPutFactSlot(
  void *theEnv,
  void *vTheFact,
  const char *slotName,
  DATA_OBJECT *theValue)
  {
   struct fact *theFact = (struct fact *) vTheFact;
   struct deftemplate *theDeftemplate;
   struct templateSlot *theSlot;
   short whichSlot;

   /*===============================================*/
   /* Get the deftemplate associated with the fact. */
   /*===============================================*/

   theDeftemplate = theFact->whichDeftemplate;

   /*============================================*/
   /* Handle setting the slot value of a fact    */
   /* having an implied deftemplate. An implied  */
   /* facts has a single multifield slot.        */
   /*============================================*/

   if (theDeftemplate->implied)
     {
      if ((slotName != NULL) || (theValue->type != MULTIFIELD))
        { return(false); }

      if (theFact->theProposition.theFields[0].type == MULTIFIELD)
        { ReturnMultifield(theEnv,(struct multifield *) theFact->theProposition.theFields[0].value); }

      theFact->theProposition.theFields[0].type = theValue->type;
      theFact->theProposition.theFields[0].value = DOToMultifield(theEnv,theValue);
      
      return(true);
     }

   /*===================================*/
   /* Make sure the slot name requested */
   /* corresponds to a valid slot name. */
   /*===================================*/

   if ((theSlot = FindSlot(theDeftemplate,(SYMBOL_HN *) EnvAddSymbol(theEnv,slotName),&whichSlot)) == NULL)
     { return(false); }

   /*=============================================*/
   /* Make sure a single field value is not being */
   /* stored in a multifield slot or vice versa.  */
   /*=============================================*/

   if (((theSlot->multislot == 0) && (theValue->type == MULTIFIELD)) ||
       ((theSlot->multislot == 1) && (theValue->type != MULTIFIELD)))
     { return(false); }

   /*=====================*/
   /* Set the slot value. */
   /*=====================*/

   if (theFact->theProposition.theFields[whichSlot-1].type == MULTIFIELD)
     { ReturnMultifield(theEnv,(struct multifield *) theFact->theProposition.theFields[whichSlot-1].value); }

   theFact->theProposition.theFields[whichSlot-1].type = theValue->type;

   if (theValue->type == MULTIFIELD)
     { theFact->theProposition.theFields[whichSlot-1].value = DOToMultifield(theEnv,theValue); }
   else
     { theFact->theProposition.theFields[whichSlot-1].value = theValue->value; }
   
   return(true);
  }

/********************************************************/
/* EnvAssignFactSlotDefaults: Sets a fact's slot values */
/*   to its default value if the value of the slot has  */
/*   not yet been set.                                  */
/********************************************************/
bool EnvAssignFactSlotDefaults(
  void *theEnv,
  void *vTheFact)
  {
   struct fact *theFact = (struct fact *) vTheFact;
   struct deftemplate *theDeftemplate;
   struct templateSlot *slotPtr;
   int i;
   DATA_OBJECT theResult;

   /*===============================================*/
   /* Get the deftemplate associated with the fact. */
   /*===============================================*/

   theDeftemplate = theFact->whichDeftemplate;

   /*================================================*/
   /* The value for the implied multifield slot of   */
   /* an implied deftemplate is set to a multifield  */
   /* of length zero when the fact is created.       */
   /*================================================*/

   if (theDeftemplate->implied) return(true);

   /*============================================*/
   /* Loop through each slot of the deftemplate. */
   /*============================================*/

   for (i = 0, slotPtr = theDeftemplate->slotList;
        i < (int) theDeftemplate->numberOfSlots;
        i++, slotPtr = slotPtr->next)
     {
      /*===================================*/
      /* If the slot's value has been set, */
      /* then move on to the next slot.    */
      /*===================================*/

      if (theFact->theProposition.theFields[i].type != RVOID) continue;

      /*======================================================*/
      /* Assign the default value for the slot if one exists. */
      /*======================================================*/
      
      if (DeftemplateSlotDefault(theEnv,theDeftemplate,slotPtr,&theResult,false))
        {
         theFact->theProposition.theFields[i].type = theResult.type;
         theFact->theProposition.theFields[i].value = theResult.value;
        }
     }

   /*==========================================*/
   /* Return true to indicate that the default */
   /* values have been successfully set.       */
   /*==========================================*/

   return(true);
  }
  
/********************************************************/
/* DeftemplateSlotDefault: Determines the default value */
/*   for the specified slot of a deftemplate.           */
/********************************************************/
bool DeftemplateSlotDefault(
  void *theEnv,
  struct deftemplate *theDeftemplate,
  struct templateSlot *slotPtr,
  DATA_OBJECT *theResult,
  bool garbageMultifield)
  {
   /*================================================*/
   /* The value for the implied multifield slot of an */
   /* implied deftemplate does not have a default.    */
   /*=================================================*/

   if (theDeftemplate->implied) return(false);

   /*===============================================*/
   /* If the (default ?NONE) attribute was declared */
   /* for the slot, then return false to indicate   */
   /* the default values for the fact couldn't be   */
   /* supplied since this attribute requires that a */
   /* default value can't be used for the slot.     */
   /*===============================================*/

   if (slotPtr->noDefault) return(false);

   /*==============================================*/
   /* Otherwise if a static default was specified, */
   /* use this as the default value.               */
   /*==============================================*/

   else if (slotPtr->defaultPresent)
     {
      if (slotPtr->multislot)
        {
         StoreInMultifield(theEnv,theResult,slotPtr->defaultList,garbageMultifield);
        }
      else
        {
         theResult->type = slotPtr->defaultList->type;
         theResult->value = slotPtr->defaultList->value;
        }
     }

   /*================================================*/
   /* Otherwise if a dynamic-default was specified,  */
   /* evaluate it and use this as the default value. */
   /*================================================*/

   else if (slotPtr->defaultDynamic)
     {
      if (! EvaluateAndStoreInDataObject(theEnv,(int) slotPtr->multislot,
                                         (EXPRESSION *) slotPtr->defaultList,
                                         theResult,garbageMultifield))
        { return(false); }
     }

   /*====================================*/
   /* Otherwise derive the default value */
   /* from the slot's constraints.       */
   /*====================================*/

   else
     {
      DeriveDefaultFromConstraints(theEnv,slotPtr->constraints,theResult,
                                  (int) slotPtr->multislot,garbageMultifield);
     }

   /*==========================================*/
   /* Return true to indicate that the default */
   /* values have been successfully set.       */
   /*==========================================*/

   return(true);
  }

/***************************************************************/
/* CopyFactSlotValues: Copies the slot values from one fact to */
/*   another. Both facts must have the same relation name.     */
/***************************************************************/
bool CopyFactSlotValues(
  void *theEnv,
  void *vTheDestFact,
  void *vTheSourceFact)
  {
   struct fact *theDestFact = (struct fact *) vTheDestFact;
   struct fact *theSourceFact = (struct fact *) vTheSourceFact;
   struct deftemplate *theDeftemplate;
   struct templateSlot *slotPtr;
   int i;

   /*===================================*/
   /* Both facts must be the same type. */
   /*===================================*/

   theDeftemplate = theSourceFact->whichDeftemplate;
   if (theDestFact->whichDeftemplate != theDeftemplate)
     { return(false); }

   /*===================================================*/
   /* Loop through each slot of the deftemplate copying */
   /* the source fact value to the destination fact.    */
   /*===================================================*/

   for (i = 0, slotPtr = theDeftemplate->slotList;
        i < (int) theDeftemplate->numberOfSlots;
        i++, slotPtr = slotPtr->next)
     {
      theDestFact->theProposition.theFields[i].type =
         theSourceFact->theProposition.theFields[i].type;
      if (theSourceFact->theProposition.theFields[i].type != MULTIFIELD)
        {
         theDestFact->theProposition.theFields[i].value =
           theSourceFact->theProposition.theFields[i].value;
        }
      else
        {
         theDestFact->theProposition.theFields[i].value =
           CopyMultifield(theEnv,(struct multifield *) theSourceFact->theProposition.theFields[i].value);
        }
     }

   /*========================================*/
   /* Return true to indicate that fact slot */
   /* values were successfully copied.       */
   /*========================================*/

   return(true);
  }

/*********************************************/
/* CreateFactBySize: Allocates a fact data   */
/*   structure based on the number of slots. */
/*********************************************/
struct fact *CreateFactBySize(
  void *theEnv,
  unsigned size)
  {
   struct fact *theFact;
   unsigned newSize;

   if (size <= 0) newSize = 1;
   else newSize = size;

   theFact = get_var_struct(theEnv,fact,sizeof(struct field) * (newSize - 1));

   theFact->garbage = false;
   theFact->factIndex = 0LL;
   theFact->factHeader.busyCount = 0;
   theFact->factHeader.theInfo = &FactData(theEnv)->FactInfo;
   theFact->factHeader.dependents = NULL;
   theFact->whichDeftemplate = NULL;
   theFact->nextFact = NULL;
   theFact->previousFact = NULL;
   theFact->previousTemplateFact = NULL;
   theFact->nextTemplateFact = NULL;
   theFact->list = NULL;
   theFact->basisSlots = NULL;

   theFact->theProposition.multifieldLength = size;
   theFact->theProposition.busyCount = 0;

   return(theFact);
  }

/*********************************************/
/* ReturnFact: Returns a fact data structure */
/*   to the pool of free memory.             */
/*********************************************/
void ReturnFact(
  void *theEnv,
  struct fact *theFact)
  {
   struct multifield *theSegment, *subSegment;
   long newSize, i;

   theSegment = &theFact->theProposition;

   for (i = 0; i < theSegment->multifieldLength; i++)
     {
      if (theSegment->theFields[i].type == MULTIFIELD)
        {
         subSegment = (struct multifield *) theSegment->theFields[i].value;
         if (subSegment->busyCount == 0)
           { ReturnMultifield(theEnv,subSegment); }
         else
           { AddToMultifieldList(theEnv,subSegment); }
        }
     }

   if (theFact->theProposition.multifieldLength == 0) newSize = 1;
   else newSize = theFact->theProposition.multifieldLength;
      
   rtn_var_struct(theEnv,fact,sizeof(struct field) * (newSize - 1),theFact);
  }

/*************************************************************/
/* FactInstall: Increments the fact, deftemplate, and atomic */
/*   data value busy counts associated with the fact.        */
/*************************************************************/
void FactInstall(
  void *theEnv,
  struct fact *newFact)
  {
   struct multifield *theSegment;
   int i;

   FactData(theEnv)->NumberOfFacts++;
   newFact->whichDeftemplate->busyCount++;
   theSegment = &newFact->theProposition;

   for (i = 0 ; i < (int) theSegment->multifieldLength ; i++)
     {
      AtomInstall(theEnv,theSegment->theFields[i].type,theSegment->theFields[i].value);
     }

   newFact->factHeader.busyCount++;
  }

/***************************************************************/
/* FactDeinstall: Decrements the fact, deftemplate, and atomic */
/*   data value busy counts associated with the fact.          */
/***************************************************************/
void FactDeinstall(
  void *theEnv,
  struct fact *newFact)
  {
   struct multifield *theSegment;
   int i;

   FactData(theEnv)->NumberOfFacts--;
   theSegment = &newFact->theProposition;
   newFact->whichDeftemplate->busyCount--;

   for (i = 0 ; i < (int) theSegment->multifieldLength ; i++)
     {
      AtomDeinstall(theEnv,theSegment->theFields[i].type,theSegment->theFields[i].value);
     }

   newFact->factHeader.busyCount--;
  }

/************************************************/
/* EnvIncrementFactCount: Increments the number */
/*   of references to a specified fact.         */
/************************************************/
void EnvIncrementFactCount(
  void *theEnv,
  void *factPtr)
  {
#if MAC_XCD
#pragma unused(theEnv)
#endif

   ((struct fact *) factPtr)->factHeader.busyCount++;
  }

/************************************************/
/* EnvDecrementFactCount: Decrements the number */
/*   of references to a specified fact.         */
/************************************************/
void EnvDecrementFactCount(
  void *theEnv,
  void *factPtr)
  {
#if MAC_XCD
#pragma unused(theEnv)
#endif

   ((struct fact *) factPtr)->factHeader.busyCount--;
  }

/*********************************************************/
/* EnvGetNextFact: If passed a NULL pointer, returns the */
/*   first fact in the fact-list. Otherwise returns the  */
/*   next fact following the fact passed as an argument. */
/*********************************************************/
void *EnvGetNextFact(
  void *theEnv,
  void *factPtr)
  {
   if (factPtr == NULL)
     { return((void *) FactData(theEnv)->FactList); }

   if (((struct fact *) factPtr)->garbage) return(NULL);

   return((void *) ((struct fact *) factPtr)->nextFact);
  }

/**************************************************/
/* GetNextFactInScope: Returns the next fact that */
/*   is in scope of the current module. Works in  */
/*   a similar fashion to GetNextFact, but skips  */
/*   facts that are out of scope.                 */
/**************************************************/
void *GetNextFactInScope(
  void *theEnv,
  void *vTheFact)
  {
   struct fact *theFact = (struct fact *) vTheFact;

   /*=======================================================*/
   /* If fact passed as an argument is a NULL pointer, then */
   /* we're just beginning a traversal of the fact list. If */
   /* the module index has changed since that last time the */
   /* fact list was traversed by this routine, then         */
   /* determine all of the deftemplates that are in scope   */
   /* of the current module.                                */
   /*=======================================================*/

   if (theFact == NULL)
     {
      theFact = FactData(theEnv)->FactList;
      if (FactData(theEnv)->LastModuleIndex != DefmoduleData(theEnv)->ModuleChangeIndex)
        {
         UpdateDeftemplateScope(theEnv);
         FactData(theEnv)->LastModuleIndex = DefmoduleData(theEnv)->ModuleChangeIndex;
        }
     }

   /*==================================================*/
   /* Otherwise, if the fact passed as an argument has */
   /* been retracted, then there's no way to determine */
   /* the next fact, so return a NULL pointer.         */
   /*==================================================*/

   else if (((struct fact *) theFact)->garbage)
     { return(NULL); }

   /*==================================================*/
   /* Otherwise, start the search for the next fact in */
   /* scope with the fact immediately following the    */
   /* fact passed as an argument.                      */
   /*==================================================*/

   else
     { theFact = theFact->nextFact; }

   /*================================================*/
   /* Continue traversing the fact-list until a fact */
   /* is found that's associated with a deftemplate  */
   /* that's in scope.                               */
   /*================================================*/

   while (theFact != NULL)
     {
      if (theFact->whichDeftemplate->inScope) return((void *) theFact);

      theFact = theFact->nextFact;
     }

   return(NULL);
  }

/****************************************/
/* EnvGetFactPPForm: Returns the pretty */
/*   print representation of a fact.    */
/****************************************/
void EnvGetFactPPForm(
  void *theEnv,
  char *buffer,
  size_t bufferLength,
  void *theFact)
  {
   OpenStringDestination(theEnv,"FactPPForm",buffer,bufferLength);
   PrintFactWithIdentifier(theEnv,"FactPPForm",(struct fact *) theFact,NULL);
   CloseStringDestination(theEnv,"FactPPForm");
  }

/**********************************/
/* EnvFactIndex: C access routine */
/*   for the fact-index function. */
/**********************************/
long long EnvFactIndex(
  void *theEnv,
  void *factPtr)
  {
#if MAC_XCD
#pragma unused(theEnv)
#endif

   return(((struct fact *) factPtr)->factIndex);
  }

/*************************************/
/* EnvAssertString: C access routine */
/*   for the assert-string function. */
/*************************************/
void *EnvAssertString(
  void *theEnv,
  const char *theString)
  {
   struct fact *theFact;
   int danglingConstructs;
   danglingConstructs = ConstructData(theEnv)->DanglingConstructs;

   if ((theFact = StringToFact(theEnv,theString)) == NULL) return(NULL);
   
   if ((! CommandLineData(theEnv)->EvaluatingTopLevelCommand) &&
       (EvaluationData(theEnv)->CurrentExpression == NULL))
     { ConstructData(theEnv)->DanglingConstructs = danglingConstructs; }

   return((void *) EnvAssert(theEnv,(void *) theFact));
  }

/******************************************************/
/* EnvGetFactListChanged: Returns the flag indicating */
/*   whether a change to the fact-list has been made. */
/******************************************************/
bool EnvGetFactListChanged(
  void *theEnv)
  {
   return(FactData(theEnv)->ChangeToFactList); 
  }

/***********************************************************/
/* EnvSetFactListChanged: Sets the flag indicating whether */
/*   a change to the fact-list has been made.              */
/***********************************************************/
void EnvSetFactListChanged(
  void *theEnv,
  bool value)
  {
   FactData(theEnv)->ChangeToFactList = value;
  }

/****************************************/
/* GetNumberOfFacts: Returns the number */
/* of facts in the fact-list.           */
/****************************************/
unsigned long GetNumberOfFacts(
  void *theEnv)
  {   
   return(FactData(theEnv)->NumberOfFacts); 
  }

/***********************************************************/
/* ResetFacts: Reset function for facts. Sets the starting */
/*   fact index to zero and removes all facts.             */
/***********************************************************/
static void ResetFacts(
  void *theEnv)
  {
   /*====================================*/
   /* Initialize the fact index to zero. */
   /*====================================*/

   FactData(theEnv)->NextFactIndex = 1L;

   /*======================================*/
   /* Remove all facts from the fact list. */
   /*======================================*/

   RemoveAllFacts(theEnv);
  }

/************************************************************/
/* ClearFactsReady: Clear ready function for facts. Returns */
/*   true if facts were successfully removed and the clear  */
/*   command can continue, otherwise false.                 */
/************************************************************/
static bool ClearFactsReady(
  void *theEnv)
  {
   /*======================================*/
   /* Facts can not be deleted when a join */
   /* operation is already in progress.    */
   /*======================================*/

   if (EngineData(theEnv)->JoinOperationInProgress) return(false);
   
   /*====================================*/
   /* Initialize the fact index to zero. */
   /*====================================*/

   FactData(theEnv)->NextFactIndex = 1L;

   /*======================================*/
   /* Remove all facts from the fact list. */
   /*======================================*/

   RemoveAllFacts(theEnv);

   /*==============================================*/
   /* If for some reason there are any facts still */
   /* remaining, don't continue with the clear.    */
   /*==============================================*/

   if (EnvGetNextFact(theEnv,NULL) != NULL) return(false);

   /*=============================*/
   /* Return true to indicate the */
   /* clear command can continue. */
   /*=============================*/

   return(true);
  }

/***************************************************/
/* FindIndexedFact: Returns a pointer to a fact in */
/*   the fact list with the specified fact index.  */
/***************************************************/
struct fact *FindIndexedFact(
  void *theEnv,
  long long factIndexSought)
  {
   struct fact *theFact;

   for (theFact = (struct fact *) EnvGetNextFact(theEnv,NULL);
        theFact != NULL;
        theFact = (struct fact *) EnvGetNextFact(theEnv,theFact))
     {
      if (theFact->factIndex == factIndexSought)
        { return(theFact); }
     }

   return(NULL);
  }

/*****************************************/
/* EnvAddAssertFunction: Adds a function */
/*   to the ListOfAssertFunctions.       */
/*****************************************/
bool EnvAddAssertFunction(
  void *theEnv,
  const char *name,
  void (*functionPtr)(void *, void *),
  int priority)
  {
   FactData(theEnv)->ListOfAssertFunctions =
      AddFunctionToCallListWithArg(theEnv,name,priority,
                                              functionPtr,
                                              FactData(theEnv)->ListOfAssertFunctions,true);
   return(1);
  }
    
/********************************************/
/* EnvAddAssertFunctionWithContext: Adds a  */
/*   function to the ListOfAssertFunctions. */
/********************************************/
bool EnvAddAssertFunctionWithContext(
  void *theEnv,
  const char *name,
  void (*functionPtr)(void *, void *),
  int priority,
  void *context)
  {
   FactData(theEnv)->ListOfAssertFunctions =
      AddFunctionToCallListWithArgWithContext(theEnv,name,priority,functionPtr,
                                       FactData(theEnv)->ListOfAssertFunctions,
                                       true,context);
   return(1);
  }
    
/***********************************************/
/* EnvRemoveAssertFunction: Removes a function */
/*   from the ListOfAssertFunctions.           */
/***********************************************/
bool EnvRemoveAssertFunction(
  void *theEnv,
  const char *name)
  {
   bool found;

   FactData(theEnv)->ListOfAssertFunctions =
      RemoveFunctionFromCallListWithArg(theEnv,name,FactData(theEnv)->ListOfAssertFunctions,&found);

   if (found) return(true);

   return(false);
  }
  
/******************************************/
/* EnvAddRetractFunction: Adds a function */
/*   to the ListOfRetractFunctions.       */
/******************************************/
bool EnvAddRetractFunction(
  void *theEnv,
  const char *name,
  void (*functionPtr)(void *, void *),
  int priority)
  {
   FactData(theEnv)->ListOfRetractFunctions =
      AddFunctionToCallListWithArg(theEnv,name,priority,
                                              functionPtr,
                                              FactData(theEnv)->ListOfRetractFunctions,true);
   return(1);
  }
    
/*********************************************/
/* EnvAddRetractFunctionWithContext: Adds a  */
/*   function to the ListOfRetractFunctions. */
/*********************************************/
bool EnvAddRetractFunctionWithContext(
  void *theEnv,
  const char *name,
  void (*functionPtr)(void *, void *),
  int priority,
  void *context)
  {
   FactData(theEnv)->ListOfRetractFunctions =
      AddFunctionToCallListWithArgWithContext(theEnv,name,priority,functionPtr,
                                       FactData(theEnv)->ListOfRetractFunctions,
                                       true,context);
   return(1);
  }
    
/************************************************/
/* EnvRemoveRetractFunction: Removes a function */
/*   from the ListOfRetractFunctions.           */
/************************************************/
bool EnvRemoveRetractFunction(
  void *theEnv,
  const char *name)
  {
   bool found;

   FactData(theEnv)->ListOfRetractFunctions =
      RemoveFunctionFromCallListWithArg(theEnv,name,FactData(theEnv)->ListOfRetractFunctions,&found);

   if (found) return(true);

   return(false);
  }

/*****************************************/
/* EnvAddModifyFunction: Adds a function */
/*   to the ListOfModifyFunctions.       */
/*****************************************/
bool EnvAddModifyFunction(
  void *theEnv,
  const char *name,
  void (*functionPtr)(void *, void *, void *),
  int priority)
  {
   FactData(theEnv)->ListOfModifyFunctions =
      AddFunctionToCallListWithArg(theEnv,name,priority,
                                              (void (*)(void *, void *)) functionPtr,
                                              FactData(theEnv)->ListOfModifyFunctions,true);
   return(1);
  }
    
/********************************************/
/* EnvAddModifyFunctionWithContext: Adds a  */
/*   function to the ListOfModifyFunctions. */
/********************************************/
bool EnvAddModifyFunctionWithContext(
  void *theEnv,
  const char *name,
  void (*functionPtr)(void *, void *, void *),
  int priority,
  void *context)
  {
   FactData(theEnv)->ListOfModifyFunctions =
      AddFunctionToCallListWithArgWithContext(theEnv,name,priority,
                                       (void (*)(void *, void *)) functionPtr,
                                       FactData(theEnv)->ListOfModifyFunctions,
                                       true,context);
   return(1);
  }
    
/***********************************************/
/* EnvRemoveModifyFunction: Removes a function */
/*   from the ListOfModifyFunctions.           */
/***********************************************/
bool EnvRemoveModifyFunction(
  void *theEnv,
  const char *name)
  {
   bool found;

   FactData(theEnv)->ListOfModifyFunctions =
      RemoveFunctionFromCallListWithArg(theEnv,name,FactData(theEnv)->ListOfModifyFunctions,&found);

   if (found) return(true);

   return(false);
  }

/*#####################################*/
/* ALLOW_ENVIRONMENT_GLOBALS Functions */
/*#####################################*/

#if ALLOW_ENVIRONMENT_GLOBALS

bool AddAssertFunction(
  const char *name,
  void (*functionPtr)(void *,void *),
  int priority)
  {
   void *theEnv;
   
   theEnv = GetCurrentEnvironment();

   FactData(theEnv)->ListOfAssertFunctions =
       AddFunctionToCallListWithArg(theEnv,name,priority,(void (*)(void *, void *)) functionPtr,
                             FactData(theEnv)->ListOfAssertFunctions,true);
   return(1);
  }

bool AddModifyFunction(
  const char *name,
  void (*functionPtr)(void *,void *,void *),
  int priority)
  {
   void *theEnv;
   
   theEnv = GetCurrentEnvironment();

   FactData(theEnv)->ListOfModifyFunctions =
       AddFunctionToCallListWithArg(theEnv,name,priority,(void (*)(void *, void *)) functionPtr,
                             FactData(theEnv)->ListOfModifyFunctions,true);
   return(1);
  }

bool AddRetractFunction(
  const char *name,
  void (*functionPtr)(void *,void *),
  int priority)
  {
   void *theEnv;
   
   theEnv = GetCurrentEnvironment();

   FactData(theEnv)->ListOfRetractFunctions =
       AddFunctionToCallListWithArg(theEnv,name,priority,(void (*)(void *, void *)) functionPtr,
                             FactData(theEnv)->ListOfRetractFunctions,true);
   return(1);
  }

void *Assert(
  void *vTheFact)
  {
   return EnvAssert(GetCurrentEnvironment(),vTheFact);
  }

void *AssertString(
  const char *theString)
  {
   return EnvAssertString(GetCurrentEnvironment(),theString);
  }

bool AssignFactSlotDefaults(
  void *vTheFact)
  {
   return EnvAssignFactSlotDefaults(GetCurrentEnvironment(),vTheFact);
  }

struct fact *CreateFact(
  void *vTheDeftemplate)
  {
   return EnvCreateFact(GetCurrentEnvironment(),vTheDeftemplate);
  }

void DecrementFactCount(
  void *factPtr)
  {
   EnvDecrementFactCount(GetCurrentEnvironment(),factPtr);
  }

long long FactIndex(
  void *factPtr)
  {
   return(EnvFactIndex(GetCurrentEnvironment(),factPtr));
  }

bool GetFactListChanged()
  {
   return EnvGetFactListChanged(GetCurrentEnvironment());
  }

void GetFactPPForm(
  char *buffer,
  size_t bufferLength,
  void *theFact)
  {
   EnvGetFactPPForm(GetCurrentEnvironment(),buffer,bufferLength,theFact);
  }

bool GetFactSlot(
  void *vTheFact,
  const char *slotName,
  DATA_OBJECT *theValue)
  {
   return(EnvGetFactSlot(GetCurrentEnvironment(),vTheFact,slotName,theValue));
  }

void *GetNextFact(
  void *factPtr)
  {
   return EnvGetNextFact(GetCurrentEnvironment(),factPtr);
  }

void IncrementFactCount(
  void *factPtr)
  {
   EnvIncrementFactCount(GetCurrentEnvironment(),factPtr);
  }

bool PutFactSlot(
  void *vTheFact,
  const char *slotName,
  DATA_OBJECT *theValue)
  {
   return EnvPutFactSlot(GetCurrentEnvironment(),vTheFact,slotName,theValue);
  }

bool RemoveAssertFunction(
  const char *name)
  {
   return EnvRemoveAssertFunction(GetCurrentEnvironment(),name);
  }

bool RemoveModifyFunction(
  const char *name)
  {
   return EnvRemoveModifyFunction(GetCurrentEnvironment(),name);
  }

bool RemoveRetractFunction(
  const char *name)
  {
   return EnvRemoveRetractFunction(GetCurrentEnvironment(),name);
  }

bool Retract(
  void *vTheFact)
  {
   return EnvRetract(GetCurrentEnvironment(),vTheFact);
  }

void SetFactListChanged(
  bool value)
  {
   EnvSetFactListChanged(GetCurrentEnvironment(),value);
  }

#endif /* ALLOW_ENVIRONMENT_GLOBALS */

#endif /* DEFTEMPLATE_CONSTRUCT && DEFRULE_CONSTRUCT */

