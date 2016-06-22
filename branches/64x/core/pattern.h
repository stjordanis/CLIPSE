   /*******************************************************/
   /*      "C" Language Integrated Production System      */
   /*                                                     */
   /*             CLIPS Version 6.40  06/20/16            */
   /*                                                     */
   /*                PATTERN HEADER FILE                  */
   /*******************************************************/

/*************************************************************/
/* Purpose: Provides the mechanism for recognizing and       */
/*   parsing the various types of patterns that can be used  */
/*   in the LHS of a rule. In version 6.0, the only pattern  */
/*   types provided are for deftemplate and instance         */
/*   patterns.                                               */
/*                                                           */
/* Principal Programmer(s):                                  */
/*      Gary D. Riley                                        */
/*                                                           */
/* Contributing Programmer(s):                               */
/*                                                           */
/* Revision History:                                         */
/*                                                           */
/*      6.24: Removed LOGICAL_DEPENDENCIES compilation flag. */
/*                                                           */
/*            Renamed BOOLEAN macro type to intBool.         */
/*                                                           */
/*      6.30: Added support for hashed alpha memories.       */
/*                                                           */
/*            Added const qualifiers to remove C++           */
/*            deprecation warnings.                          */
/*                                                           */
/*      6.40: Removed LOCALE definition.                     */
/*                                                           */
/*************************************************************/

#ifndef _H_pattern

#define _H_pattern

#ifndef _STDIO_INCLUDED_
#include <stdio.h>
#define _STDIO_INCLUDED_
#endif

#ifndef _H_evaluatn
#include "evaluatn.h"
#endif

struct patternEntityRecord
  {
   struct entityRecord base;
   void (*decrementBasisCount)(void *,void *);
   void (*incrementBasisCount)(void *,void *);
   void (*matchFunction)(void *,void *);
   intBool (*synchronized)(void *,void *);
   intBool (*isDeleted)(void *,void *);
  };

typedef struct patternEntityRecord PTRN_ENTITY_RECORD;
typedef struct patternEntityRecord *PTRN_ENTITY_RECORD_PTR;

struct patternEntity
  {
   struct patternEntityRecord *theInfo;
   void *dependents;
   unsigned busyCount;
   unsigned long long timeTag;
  };

typedef struct patternEntity PATTERN_ENTITY;
typedef struct patternEntity * PATTERN_ENTITY_PTR;

struct patternParser;

#ifndef _H_symbol
#include "symbol.h"
#endif
#ifndef _H_scanner
#include "scanner.h"
#endif
#ifndef _H_expressn
#include "expressn.h"
#endif
#ifndef _H_match
#include "match.h"
#endif
#ifndef _H_reorder
#include "reorder.h"
#endif
#ifndef _H_constrnt
#include "constrnt.h"
#endif

#define MAXIMUM_NUMBER_OF_PATTERNS 128

struct patternParser
  {
   const char *name;
   struct patternEntityRecord *entityType;
   int positionInArray;
   int (*recognizeFunction)(SYMBOL_HN *);
   struct lhsParseNode *(*parseFunction)(void *,const char *,struct token *);
   int (*postAnalysisFunction)(void *,struct lhsParseNode *);
   struct patternNodeHeader *(*addPatternFunction)(void *,struct lhsParseNode *);
   void (*removePatternFunction)(void *,struct patternNodeHeader *);
   struct expr *(*genJNConstantFunction)(void *,struct lhsParseNode *,int);
   void (*replaceGetJNValueFunction)(void *,struct expr *,struct lhsParseNode *,int);
   struct expr *(*genGetJNValueFunction)(void *,struct lhsParseNode *,int);
   struct expr *(*genCompareJNValuesFunction)(void *,struct lhsParseNode *,struct lhsParseNode *,int);
   struct expr *(*genPNConstantFunction)(void *,struct lhsParseNode *);
   void (*replaceGetPNValueFunction)(void *,struct expr *,struct lhsParseNode *);
   struct expr *(*genGetPNValueFunction)(void *,struct lhsParseNode *);
   struct expr *(*genComparePNValuesFunction)(void *,struct lhsParseNode *,struct lhsParseNode *);
   void (*returnUserDataFunction)(void *,void *);
   void *(*copyUserDataFunction)(void *,void *);
   void (*markIRPatternFunction)(void *,struct patternNodeHeader *,int);
   void (*incrementalResetFunction)(void *);
   struct lhsParseNode *(*initialPatternFunction)(void *);
   void (*codeReferenceFunction)(void *,void *,FILE *,int,int);
   int priority;
   struct patternParser *next;
  };

struct reservedSymbol
  {
   const char *theSymbol;
   const char *reservedBy;
   struct reservedSymbol *next;
  };

#define MAX_POSITIONS 8

#define PATTERN_DATA 19

struct patternData
  { 
   struct patternParser *ListOfPatternParsers;
   struct patternParser *PatternParserArray[MAX_POSITIONS];
   int NextPosition;
   struct reservedSymbol *ListOfReservedPatternSymbols;
   int WithinNotCE;
   int  GlobalSalience;
   int GlobalAutoFocus;
   struct expr *SalienceExpression;
   struct patternNodeHashEntry **PatternHashTable;
   unsigned long PatternHashTableSize;
  };

#define PatternData(theEnv) ((struct patternData *) GetEnvironmentData(theEnv,PATTERN_DATA))

   void                           InitializePatterns(void *);
   int                            AddPatternParser(void *,struct patternParser *);
   struct patternParser          *FindPatternParser(void *,const char *);
   void                           DetachPattern(void *,int,struct patternNodeHeader *);
   void                           GetNextPatternEntity(void *,
                                                              struct patternParser **,
                                                              struct patternEntity **);
   struct patternParser          *GetPatternParser(void *,int);
   struct lhsParseNode           *RestrictionParse(void *,const char *,struct token *,int,
                                                       struct symbolHashNode *,short,
                                                       struct constraintRecord *,short);
   int                            PostPatternAnalysis(void *,struct lhsParseNode *);
   void                           PatternNodeHeaderToCode(void *,FILE *,struct patternNodeHeader *,int,int);
   void                           AddReservedPatternSymbol(void *,const char *,const char *);
   intBool                        ReservedPatternSymbol(void *,const char *,const char *);
   void                           ReservedPatternSymbolErrorMsg(void *,const char *,const char *);
   void                           AddHashedPatternNode(void *,void *,void *,unsigned short,void *);
   intBool                        RemoveHashedPatternNode(void *,void *,void *,unsigned short,void *);
   void                          *FindHashedPatternNode(void *,void *,unsigned short,void *);

#endif /* _H_pattern */









