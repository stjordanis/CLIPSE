   /*******************************************************/
   /*      "C" Language Integrated Production System      */
   /*                                                     */
   /*            CLIPS Version 6.40  01/06/16             */
   /*                                                     */
   /*                DEFMODULE HEADER FILE                */
   /*******************************************************/

/*************************************************************/
/* Purpose: Defines basic defmodule primitive functions such */
/*   as allocating and deallocating, traversing, and finding */
/*   defmodule data structures.                              */
/*                                                           */
/* Principal Programmer(s):                                  */
/*      Gary D. Riley                                        */
/*                                                           */
/* Contributing Programmer(s):                               */
/*      Brian L. Dantes                                      */
/*                                                           */
/* Revision History:                                         */
/*                                                           */
/*      6.23: Correction for FalseSymbol/TrueSymbol. DR0859  */
/*                                                           */
/*            Corrected compilation errors for files         */
/*            generated by constructs-to-c. DR0861           */
/*                                                           */
/*      6.24: Renamed BOOLEAN macro type to intBool.         */
/*                                                           */
/*      6.30: Changed integer type/precision.                */
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
/*************************************************************/

#ifndef _H_moduldef
#define _H_moduldef

struct defmodule;
struct portItem;
struct defmoduleItemHeader;
struct moduleItem;

#ifndef _STDIO_INCLUDED_
#include <stdio.h>
#define _STDIO_INCLUDED_
#endif

#ifndef _H_conscomp
#include "conscomp.h"
#endif
#ifndef _H_modulpsr
#include "modulpsr.h"
#endif
#ifndef _H_utility
#include "utility.h"
#endif
#ifndef _H_symbol
#include "symbol.h"
#endif
#ifndef _H_evaluatn
#include "evaluatn.h"
#endif
#ifndef _H_constrct
#include "constrct.h"
#endif

/**********************************************************************/
/* defmodule                                                          */
/* ----------                                                         */
/* name: The name of the defmodule (stored as a reference in the      */
/*   table).                                                          */
/*                                                                    */
/* ppForm: The pretty print representation of the defmodule (used by  */
/*   the save and ppdefmodule commands).                              */
/*                                                                    */
/* itemsArray: An array of pointers to the module specific data used  */
/*   by each construct specified with the RegisterModuleItem          */
/*   function. The data pointer stored in the array is allocated by   */
/*   the allocateFunction in moduleItem data structure.               */
/*                                                                    */
/* importList: The list of items which are being imported by this     */
/*   module from other modules.                                       */
/*                                                                    */
/* next: A pointer to the next defmodule data structure.              */
/**********************************************************************/
struct defmodule
  {
   struct symbolHashNode *name;
   char *ppForm;
   struct defmoduleItemHeader **itemsArray;
   struct portItem *importList;
   struct portItem *exportList;
   bool visitedFlag;
   long bsaveID;
   struct userData *usrData;
   struct defmodule *next;
  };

struct portItem
  {
   struct symbolHashNode *moduleName;
   struct symbolHashNode *constructType;
   struct symbolHashNode *constructName;
   struct portItem *next;
  };

struct defmoduleItemHeader
  {
   struct defmodule *theModule;
   struct constructHeader *firstItem;
   struct constructHeader *lastItem;
  };

#define MIHS (struct defmoduleItemHeader *)

/**********************************************************************/
/* moduleItem                                                         */
/* ----------                                                         */
/* name: The name of the construct which can be placed in a module.   */
/*   For example, "defrule".                                          */
/*                                                                    */
/* allocateFunction: Used to allocate a data structure containing all */
/*   pertinent information related to a specific construct for a      */
/*   given module. For example, the deffacts construct stores a       */
/*   pointer to the first and last deffacts for each each module.     */
/*                                                                    */
/* freeFunction: Used to deallocate a data structure allocated by     */
/*   the allocateFunction. In addition, the freeFunction deletes      */
/*   all constructs of the specified type in the given module.        */
/*                                                                    */
/* bloadModuleReference: Used during a binary load to establish a     */
/*   link between the defmodule data structure and the data structure */
/*   containing all pertinent module information for a specific       */
/*   construct.                                                       */
/*                                                                    */
/* findFunction: Used to determine if a specified construct is in a   */
/*   specific module. The name is the specific construct is passed as */
/*   a string and the function returns a pointer to the specified     */
/*   construct if it exists.                                          */
/*                                                                    */
/* exportable: If true, then the specified construct type can be      */
/*   exported (and hence imported). If false, it can't be exported.   */
/*                                                                    */
/* next: A pointer to the next moduleItem data structure.             */
/**********************************************************************/

struct moduleItem
  {
   const char *name;
   int moduleIndex;
   void *(*allocateFunction)(void *);
   void  (*freeFunction)(void *,void *);
   void *(*bloadModuleReference)(void *,int);
   void  (*constructsToCModuleReference)(void *,FILE *,int,int,int);
   void *(*findFunction)(void *,const char *);
   struct moduleItem *next;
  };

typedef struct moduleStackItem
  {
   bool changeFlag;
   struct defmodule *theModule;
   struct moduleStackItem *next;
  } MODULE_STACK_ITEM;

#define DEFMODULE_DATA 4

struct defmoduleData
  {   
   struct moduleItem *LastModuleItem;
   struct callFunctionItem *AfterModuleChangeFunctions;
   MODULE_STACK_ITEM *ModuleStack;
   bool CallModuleChangeFunctions;
   struct defmodule *ListOfDefmodules;
   struct defmodule *CurrentModule;
   struct defmodule *LastDefmodule;
   int NumberOfModuleItems;
   struct moduleItem *ListOfModuleItems;
   long ModuleChangeIndex;
   bool MainModuleRedefinable;
#if (! RUN_TIME) && (! BLOAD_ONLY)
   struct portConstructItem *ListOfPortConstructItems;
   long NumberOfDefmodules;
   struct callFunctionItem *AfterModuleDefinedFunctions;
#endif
#if CONSTRUCT_COMPILER && (! RUN_TIME)
   struct CodeGeneratorItem *DefmoduleCodeItem;
#endif
#if (BLOAD || BLOAD_ONLY || BLOAD_AND_BSAVE) && (! RUN_TIME)
   long BNumberOfDefmodules;
   long NumberOfPortItems;
   struct portItem *PortItemArray;
   struct defmodule *DefmoduleArray;
#endif
  };
  
#define DefmoduleData(theEnv) ((struct defmoduleData *) GetEnvironmentData(theEnv,DEFMODULE_DATA))

   void                           InitializeDefmodules(void *);
   void                          *EnvFindDefmodule(void *,const char *);
   const char                    *EnvGetDefmoduleName(void *,void *);
   const char                    *EnvGetDefmodulePPForm(void *,void *);
   void                          *EnvGetNextDefmodule(void *,void *);
   void                           RemoveAllDefmodules(void *);
   int                            AllocateModuleStorage(void);
   int                            RegisterModuleItem(void *,const char *,
                                                            void *(*)(void *),
                                                            void (*)(void *,void *),
                                                            void *(*)(void *,int),
                                                            void (*)(void *,FILE *,int,int,int),
                                                            void *(*)(void *,const char *));
   void                          *GetModuleItem(void *,struct defmodule *,int);
   void                           SetModuleItem(void *,struct defmodule *,int,void *);
   void                          *EnvGetCurrentModule(void *);
   void                          *EnvSetCurrentModule(void *,void *);
   void                          *GetCurrentModuleCommand(void *);
   void                          *SetCurrentModuleCommand(void *);
   int                            GetNumberOfModuleItems(void *);
   void                           CreateMainModule(void *);
   void                           SetListOfDefmodules(void *,void *);
   struct moduleItem             *GetListOfModuleItems(void *);
   struct moduleItem             *FindModuleItem(void *,const char *);
   void                           SaveCurrentModule(void *);
   void                           RestoreCurrentModule(void *);
   void                           AddAfterModuleChangeFunction(void *,const char *,void (*)(void *),int);
   void                           IllegalModuleSpecifierMessage(void *);
   void                           AllocateDefmoduleGlobals(void *);
   long                           GetNumberOfDefmodules(void *);

#endif /* _H_moduldef */


