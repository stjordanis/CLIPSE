   /*******************************************************/
   /*      "C" Language Integrated Production System      */
   /*                                                     */
   /*            CLIPS Version 6.40  01/06/16             */
   /*                                                     */
   /*                PRINT UTILITY MODULE                 */
   /*******************************************************/

/*************************************************************/
/* Purpose: Utility routines for printing various items      */
/*   and messages.                                           */
/*                                                           */
/* Principal Programmer(s):                                  */
/*      Gary D. Riley                                        */
/*                                                           */
/* Contributing Programmer(s):                               */
/*      Brian L. Dantes                                      */
/*                                                           */
/* Revision History:                                         */
/*                                                           */
/*      6.24: Link error occurs for the SlotExistError       */
/*            function when OBJECT_SYSTEM is set to 0 in     */
/*            setup.h. DR0865                                */
/*                                                           */
/*            Added DataObjectToString function.             */
/*                                                           */
/*            Added SlotExistError function.                 */
/*                                                           */
/*      6.30: Support for long long integers.                */
/*                                                           */
/*            Support for DATA_OBJECT_ARRAY primitive.       */
/*                                                           */
/*            Support for typed EXTERNAL_ADDRESS.            */
/*                                                           */
/*            Used gensprintf and genstrcat instead of       */
/*            sprintf and strcat.                            */
/*                                                           */
/*            Changed integer type/precision.                */
/*                                                           */
/*            Added code for capturing errors/warnings.      */
/*                                                           */
/*            Added const qualifiers to remove C++           */
/*            deprecation warnings.                          */
/*                                                           */
/*            Fixed linkage issue when BLOAD_ONLY compiler   */
/*            flag is set to 1.                              */
/*                                                           */
/*      6.31: Added Env prefix to GetEvaluationError and     */
/*            SetEvaluationError functions.                  */
/*                                                           */
/*************************************************************/

#include <stdio.h>
#include <string.h>

#include "setup.h"

#include "argacces.h"
#include "constant.h"
#include "cstrcpsr.h"
#include "envrnmnt.h"
#include "evaluatn.h"
#include "factmngr.h"
#include "inscom.h"
#include "insmngr.h"
#include "memalloc.h"
#include "multifun.h"
#include "router.h"
#include "symbol.h"
#include "sysdep.h"
#include "utility.h"

#include "prntutil.h"

/*****************************************************/
/* InitializePrintUtilityData: Allocates environment */
/*    data for print utility routines.               */
/*****************************************************/
void InitializePrintUtilityData(
  void *theEnv)
  {
   AllocateEnvironmentData(theEnv,PRINT_UTILITY_DATA,sizeof(struct printUtilityData),NULL);
  }

/***********************************************************/
/* PrintInChunks:  Prints a string in chunks to accomodate */
/*   systems which have a limit on the maximum size of a   */
/*   string which can be printed.                          */
/***********************************************************/
void PrintInChunks(
  void *theEnv,
  const char *logicalName,
  const char *bigString)
  {
   /*=====================================================*/
   /* This function was originally added because VMS had  */
   /* a bug that didn't allow printing a string greater   */
   /* than 512 bytes. Since this was over 25 years ago,   */
   /* we'll assume no modern compiler has this limitation */
   /* and just print the entire string.                   */
   /*=====================================================*/
   
   EnvPrintRouter(theEnv,logicalName,bigString);

/*
   char tc, *subString;

   subString = bigString;

   if (subString == NULL) return;

   while (((int) strlen(subString)) > 500)
     {
      if (EvaluationData(theEnv)->HaltExecution) return;
      tc = subString[500];
      subString[500] = EOS;
      EnvPrintRouter(theEnv,logicalName,subString);
      subString[500] = tc;
      subString += 500;
     }

   EnvPrintRouter(theEnv,logicalName,subString);
*/
  }

/************************************************************/
/* PrintFloat: Controls printout of floating point numbers. */
/************************************************************/
void PrintFloat(
  void *theEnv,
  const char *fileid,
  double number)
  {
   const char *theString;

   theString = FloatToString(theEnv,number);
   EnvPrintRouter(theEnv,fileid,theString);
  }

/****************************************************/
/* PrintLongInteger: Controls printout of integers. */
/****************************************************/
void PrintLongInteger(
  void *theEnv,
  const char *logicalName,
  long long number)
  {
   char printBuffer[32];

   gensprintf(printBuffer,"%lld",number);
   EnvPrintRouter(theEnv,logicalName,printBuffer);
  }

/**************************************/
/* PrintAtom: Prints an atomic value. */
/**************************************/
void PrintAtom(
  void *theEnv,
  const char *logicalName,
  int type,
  void *value)
  {
   struct externalAddressHashNode *theAddress;
   char buffer[20];

   switch (type)
     {
      case FLOAT:
        PrintFloat(theEnv,logicalName,ValueToDouble(value));
        break;
      case INTEGER:
        PrintLongInteger(theEnv,logicalName,ValueToLong(value));
        break;
      case SYMBOL:
        EnvPrintRouter(theEnv,logicalName,ValueToString(value));
        break;
      case STRING:
        if (PrintUtilityData(theEnv)->PreserveEscapedCharacters)
          { EnvPrintRouter(theEnv,logicalName,StringPrintForm(theEnv,ValueToString(value))); }
        else
          {
           EnvPrintRouter(theEnv,logicalName,"\"");
           EnvPrintRouter(theEnv,logicalName,ValueToString(value));
           EnvPrintRouter(theEnv,logicalName,"\"");
          }
        break;

      case DATA_OBJECT_ARRAY:
        if (PrintUtilityData(theEnv)->AddressesToStrings) EnvPrintRouter(theEnv,logicalName,"\"");
        
        EnvPrintRouter(theEnv,logicalName,"<Pointer-");
        gensprintf(buffer,"%p",value);
        EnvPrintRouter(theEnv,logicalName,buffer);
        EnvPrintRouter(theEnv,logicalName,">");
          
        if (PrintUtilityData(theEnv)->AddressesToStrings) EnvPrintRouter(theEnv,logicalName,"\"");
        break;

      case EXTERNAL_ADDRESS:
        theAddress = (struct externalAddressHashNode *) value;
        
        if (PrintUtilityData(theEnv)->AddressesToStrings) EnvPrintRouter(theEnv,logicalName,"\"");
        
        if ((EvaluationData(theEnv)->ExternalAddressTypes[theAddress->type] != NULL) &&
            (EvaluationData(theEnv)->ExternalAddressTypes[theAddress->type]->longPrintFunction != NULL))
          { (*EvaluationData(theEnv)->ExternalAddressTypes[theAddress->type]->longPrintFunction)(theEnv,logicalName,value); }
        else
          {
           EnvPrintRouter(theEnv,logicalName,"<Pointer-");
        
           gensprintf(buffer,"%d-",theAddress->type);
           EnvPrintRouter(theEnv,logicalName,buffer);
        
           gensprintf(buffer,"%p",ValueToExternalAddress(value));
           EnvPrintRouter(theEnv,logicalName,buffer);
           EnvPrintRouter(theEnv,logicalName,">");
          }
          
        if (PrintUtilityData(theEnv)->AddressesToStrings) EnvPrintRouter(theEnv,logicalName,"\"");
        break;

#if OBJECT_SYSTEM
      case INSTANCE_NAME:
        EnvPrintRouter(theEnv,logicalName,"[");
        EnvPrintRouter(theEnv,logicalName,ValueToString(value));
        EnvPrintRouter(theEnv,logicalName,"]");
        break;
#endif

      case RVOID:
        break;

      default:
        if (EvaluationData(theEnv)->PrimitivesArray[type] == NULL) break;
        if (EvaluationData(theEnv)->PrimitivesArray[type]->longPrintFunction == NULL)
          {
           EnvPrintRouter(theEnv,logicalName,"<unknown atom type>");
           break;
          }
        (*EvaluationData(theEnv)->PrimitivesArray[type]->longPrintFunction)(theEnv,logicalName,value);
        break;
     }
  }

/**********************************************************/
/* PrintTally: Prints a tally count indicating the number */
/*   of items that have been displayed. Used by functions */
/*   such as list-defrules.                               */
/**********************************************************/
void PrintTally(
  void *theEnv,
  const char *logicalName,
  long long count,
  const char *singular,
  const char *plural)
  {
   if (count == 0) return;

   EnvPrintRouter(theEnv,logicalName,"For a total of ");
   PrintLongInteger(theEnv,logicalName,count);
   EnvPrintRouter(theEnv,logicalName," ");

   if (count == 1) EnvPrintRouter(theEnv,logicalName,singular);
   else EnvPrintRouter(theEnv,logicalName,plural);

   EnvPrintRouter(theEnv,logicalName,".\n");
  }

/********************************************/
/* PrintErrorID: Prints the module name and */
/*   error ID for an error message.         */
/********************************************/
void PrintErrorID(
  void *theEnv,
  const char *module,
  int errorID,
  bool printCR)
  {
#if (! RUN_TIME) && (! BLOAD_ONLY)
   FlushParsingMessages(theEnv);
   EnvSetErrorFileName(theEnv,EnvGetParsingFileName(theEnv));
   ConstructData(theEnv)->ErrLineNumber = GetLineCount(theEnv);
#endif
   if (printCR) EnvPrintRouter(theEnv,WERROR,"\n");
   EnvPrintRouter(theEnv,WERROR,"[");
   EnvPrintRouter(theEnv,WERROR,module);
   PrintLongInteger(theEnv,WERROR,(long int) errorID);
   EnvPrintRouter(theEnv,WERROR,"] ");
  }

/**********************************************/
/* PrintWarningID: Prints the module name and */
/*   warning ID for a warning message.        */
/**********************************************/
void PrintWarningID(
  void *theEnv,
  const char *module,
  int warningID,
  bool printCR)
  {
#if (! RUN_TIME) && (! BLOAD_ONLY)
   FlushParsingMessages(theEnv);
   EnvSetWarningFileName(theEnv,EnvGetParsingFileName(theEnv));
   ConstructData(theEnv)->WrnLineNumber = GetLineCount(theEnv);
#endif
   if (printCR) EnvPrintRouter(theEnv,WWARNING,"\n");
   EnvPrintRouter(theEnv,WWARNING,"[");
   EnvPrintRouter(theEnv,WWARNING,module);
   PrintLongInteger(theEnv,WWARNING,(long int) warningID);
   EnvPrintRouter(theEnv,WWARNING,"] WARNING: ");
  }

/***************************************************/
/* CantFindItemErrorMessage: Generic error message */
/*  when an "item" can not be found.               */
/***************************************************/
void CantFindItemErrorMessage(
  void *theEnv,
  const char *itemType,
  const char *itemName)
  {
   PrintErrorID(theEnv,"PRNTUTIL",1,false);
   EnvPrintRouter(theEnv,WERROR,"Unable to find ");
   EnvPrintRouter(theEnv,WERROR,itemType);
   EnvPrintRouter(theEnv,WERROR," ");
   EnvPrintRouter(theEnv,WERROR,itemName);
   EnvPrintRouter(theEnv,WERROR,".\n");
  }

/*****************************************************/
/* CantFindItemInFunctionErrorMessage: Generic error */
/*  message when an "item" can not be found.         */
/*****************************************************/
void CantFindItemInFunctionErrorMessage(
  void *theEnv,
  const char *itemType,
  const char *itemName,
  const char *func)
  {
   PrintErrorID(theEnv,"PRNTUTIL",1,false);
   EnvPrintRouter(theEnv,WERROR,"Unable to find ");
   EnvPrintRouter(theEnv,WERROR,itemType);
   EnvPrintRouter(theEnv,WERROR," ");
   EnvPrintRouter(theEnv,WERROR,itemName);
   EnvPrintRouter(theEnv,WERROR," in function ");
   EnvPrintRouter(theEnv,WERROR,func);
   EnvPrintRouter(theEnv,WERROR,".\n");
  }

/*****************************************************/
/* CantDeleteItemErrorMessage: Generic error message */
/*  when an "item" can not be deleted.               */
/*****************************************************/
void CantDeleteItemErrorMessage(
  void *theEnv,
  const char *itemType,
  const char *itemName)
  {
   PrintErrorID(theEnv,"PRNTUTIL",4,false);
   EnvPrintRouter(theEnv,WERROR,"Unable to delete ");
   EnvPrintRouter(theEnv,WERROR,itemType);
   EnvPrintRouter(theEnv,WERROR," ");
   EnvPrintRouter(theEnv,WERROR,itemName);
   EnvPrintRouter(theEnv,WERROR,".\n");
  }

/****************************************************/
/* AlreadyParsedErrorMessage: Generic error message */
/*  when an "item" has already been parsed.         */
/****************************************************/
void AlreadyParsedErrorMessage(
  void *theEnv,
  const char *itemType,
  const char *itemName)
  {
   PrintErrorID(theEnv,"PRNTUTIL",5,true);
   EnvPrintRouter(theEnv,WERROR,"The ");
   if (itemType != NULL) EnvPrintRouter(theEnv,WERROR,itemType);
   if (itemName != NULL) EnvPrintRouter(theEnv,WERROR,itemName);
   EnvPrintRouter(theEnv,WERROR," has already been parsed.\n");
  }

/*********************************************************/
/* SyntaxErrorMessage: Generalized syntax error message. */
/*********************************************************/
void SyntaxErrorMessage(
  void *theEnv,
  const char *location)
  {
   PrintErrorID(theEnv,"PRNTUTIL",2,true);
   EnvPrintRouter(theEnv,WERROR,"Syntax Error");
   if (location != NULL)
     {
      EnvPrintRouter(theEnv,WERROR,":  Check appropriate syntax for ");
      EnvPrintRouter(theEnv,WERROR,location);
     }

   EnvPrintRouter(theEnv,WERROR,".\n");
   EnvSetEvaluationError(theEnv,true);
  }

/****************************************************/
/* LocalVariableErrorMessage: Generic error message */
/*  when a local variable is accessed by an "item"  */
/*  which can not access local variables.           */
/****************************************************/
void LocalVariableErrorMessage(
  void *theEnv,
  const char *byWhat)
  {
   PrintErrorID(theEnv,"PRNTUTIL",6,true);
   EnvPrintRouter(theEnv,WERROR,"Local variables can not be accessed by ");
   EnvPrintRouter(theEnv,WERROR,byWhat);
   EnvPrintRouter(theEnv,WERROR,".\n");
  }

/******************************************/
/* SystemError: Generalized error message */
/*   for major internal errors.           */
/******************************************/
void SystemError(
  void *theEnv,
  const char *module,
  int errorID)
  {
   PrintErrorID(theEnv,"PRNTUTIL",3,true);

   EnvPrintRouter(theEnv,WERROR,"\n*** ");
   EnvPrintRouter(theEnv,WERROR,APPLICATION_NAME);
   EnvPrintRouter(theEnv,WERROR," SYSTEM ERROR ***\n");

   EnvPrintRouter(theEnv,WERROR,"ID = ");
   EnvPrintRouter(theEnv,WERROR,module);
   PrintLongInteger(theEnv,WERROR,(long int) errorID);
   EnvPrintRouter(theEnv,WERROR,"\n");

   EnvPrintRouter(theEnv,WERROR,APPLICATION_NAME);
   EnvPrintRouter(theEnv,WERROR," data structures are in an inconsistent or corrupted state.\n");
   EnvPrintRouter(theEnv,WERROR,"This error may have occurred from errors in user defined code.\n");
   EnvPrintRouter(theEnv,WERROR,"**************************\n");
  }

/*******************************************************/
/* DivideByZeroErrorMessage: Generalized error message */
/*   for when a function attempts to divide by zero.   */
/*******************************************************/
void DivideByZeroErrorMessage(
  void *theEnv,
  const char *functionName)
  {
   PrintErrorID(theEnv,"PRNTUTIL",7,false);
   EnvPrintRouter(theEnv,WERROR,"Attempt to divide by zero in ");
   EnvPrintRouter(theEnv,WERROR,functionName);
   EnvPrintRouter(theEnv,WERROR," function.\n");
  }

/*******************************************************/
/* FloatToString: Converts number to KB string format. */
/*******************************************************/
const char *FloatToString(
  void *theEnv,
  double number)
  {
   char floatString[40];
   int i;
   char x;
   void *thePtr;

   gensprintf(floatString,"%.15g",number);

   for (i = 0; (x = floatString[i]) != '\0'; i++)
     {
      if ((x == '.') || (x == 'e'))
        {
         thePtr = EnvAddSymbol(theEnv,floatString);
         return(ValueToString(thePtr));
        }
     }

   genstrcat(floatString,".0");

   thePtr = EnvAddSymbol(theEnv,floatString);
   return(ValueToString(thePtr));
  }

/*******************************************************************/
/* LongIntegerToString: Converts long integer to KB string format. */
/*******************************************************************/
const char *LongIntegerToString(
  void *theEnv,
  long long number)
  {
   char buffer[50];
   void *thePtr;

   gensprintf(buffer,"%lld",number);

   thePtr = EnvAddSymbol(theEnv,buffer);
   return(ValueToString(thePtr));
  }

/*******************************************************************/
/* DataObjectToString: Converts a DATA_OBJECT to KB string format. */
/*******************************************************************/
const char *DataObjectToString(
  void *theEnv,
  DATA_OBJECT *theDO)
  {
   void *thePtr;
   const char *theString;
   char *newString;
   const char *prefix, *postfix;
   size_t length;
   struct externalAddressHashNode *theAddress;
   char buffer[30];
   
   switch (GetpType(theDO))
     {
      case MULTIFIELD:
         prefix = "(";
         theString = ValueToString(ImplodeMultifield(theEnv,theDO));
         postfix = ")";
         break;
         
      case STRING:
         prefix = "\"";
         theString = DOPToString(theDO);
         postfix = "\"";
         break;
         
      case INSTANCE_NAME:
         prefix = "[";
         theString = DOPToString(theDO);
         postfix = "]";
         break;
         
      case SYMBOL:
         return(DOPToString(theDO));
         
      case FLOAT:
         return(FloatToString(theEnv,DOPToDouble(theDO)));
         
      case INTEGER:
         return(LongIntegerToString(theEnv,DOPToLong(theDO)));
         
      case RVOID:
         return("");

#if OBJECT_SYSTEM
      case INSTANCE_ADDRESS:
         thePtr = DOPToPointer(theDO);

         if (thePtr == (void *) &InstanceData(theEnv)->DummyInstance)
           { return("<Dummy Instance>"); }
           
         if (((struct instance *) thePtr)->garbage)
           {
            prefix = "<Stale Instance-";
            theString = ValueToString(((struct instance *) thePtr)->name);
            postfix = ">";
           }
         else
           {
            prefix = "<Instance-";
            theString = ValueToString(GetFullInstanceName(theEnv,(INSTANCE_TYPE *) thePtr));
            postfix = ">";
           }
           
        break;
#endif
      
      case EXTERNAL_ADDRESS:
        theAddress = (struct externalAddressHashNode *) DOPToPointer(theDO);
        /* TBD Need specific routine for creating name string. */
        gensprintf(buffer,"<Pointer-%d-%p>",(int) theAddress->type,DOPToExternalAddress(theDO));
        thePtr = EnvAddSymbol(theEnv,buffer);
        return(ValueToString(thePtr));

#if DEFTEMPLATE_CONSTRUCT      
      case FACT_ADDRESS:
         if (DOPToPointer(theDO) == (void *) &FactData(theEnv)->DummyFact)
           { return("<Dummy Fact>"); }
         
         thePtr = DOPToPointer(theDO);
         gensprintf(buffer,"<Fact-%lld>",((struct fact *) thePtr)->factIndex);
         thePtr = EnvAddSymbol(theEnv,buffer);
         return(ValueToString(thePtr));
#endif
                        
      default:
         return("UNK");
     }
     
   length = strlen(prefix) + strlen(theString) + strlen(postfix) + 1;
   newString = (char *) genalloc(theEnv,length);
   newString[0] = '\0';
   genstrcat(newString,prefix);
   genstrcat(newString,theString);
   genstrcat(newString,postfix);
   thePtr = EnvAddSymbol(theEnv,newString);
   genfree(theEnv,newString,length);
   return(ValueToString(thePtr));
  }
  
/************************************************************/
/* SalienceInformationError: Error message for errors which */
/*   occur during the evaluation of a salience value.       */
/************************************************************/
void SalienceInformationError(
  void *theEnv,
  const char *constructType,
  const char *constructName)
  {
   PrintErrorID(theEnv,"PRNTUTIL",8,true);
   EnvPrintRouter(theEnv,WERROR,"This error occurred while evaluating the salience");
   if (constructName != NULL)
     {
      EnvPrintRouter(theEnv,WERROR," for ");
      EnvPrintRouter(theEnv,WERROR,constructType);
      EnvPrintRouter(theEnv,WERROR," ");
      EnvPrintRouter(theEnv,WERROR,constructName);
     }
   EnvPrintRouter(theEnv,WERROR,".\n");
  }

/**********************************************************/
/* SalienceRangeError: Error message that is printed when */
/*   a salience value does not fall between the minimum   */
/*   and maximum salience values.                         */
/**********************************************************/
void SalienceRangeError(
  void *theEnv,
  int min,
  int max)
  {
   PrintErrorID(theEnv,"PRNTUTIL",9,true);
   EnvPrintRouter(theEnv,WERROR,"Salience value out of range ");
   PrintLongInteger(theEnv,WERROR,(long int) min);
   EnvPrintRouter(theEnv,WERROR," to ");
   PrintLongInteger(theEnv,WERROR,(long int) max);
   EnvPrintRouter(theEnv,WERROR,".\n");
  }

/***************************************************************/
/* SalienceNonIntegerError: Error message that is printed when */
/*   a rule's salience does not evaluate to an integer.        */
/***************************************************************/
void SalienceNonIntegerError(
  void *theEnv)
  {
   PrintErrorID(theEnv,"PRNTUTIL",10,true);
   EnvPrintRouter(theEnv,WERROR,"Salience value must be an integer value.\n");
  }

/***************************************************/
/* SlotExistError: Prints out an appropriate error */
/*   message when a slot cannot be found for a     */
/*   function. Input to the function is the slot   */
/*   name and the function name.                   */
/***************************************************/
void SlotExistError(
  void *theEnv,
  const char *sname,
  const char *func)
  {
   PrintErrorID(theEnv,"INSFUN",3,false);
   EnvPrintRouter(theEnv,WERROR,"No such slot ");
   EnvPrintRouter(theEnv,WERROR,sname);
   EnvPrintRouter(theEnv,WERROR," in function ");
   EnvPrintRouter(theEnv,WERROR,func);
   EnvPrintRouter(theEnv,WERROR,".\n");
   EnvSetEvaluationError(theEnv,true);
  }
