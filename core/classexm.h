   /*******************************************************/
   /*      "C" Language Integrated Production System      */
   /*                                                     */
   /*            CLIPS Version 6.40  01/06/16             */
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
/*                                                           */
/*      6.23: Corrected compilation errors for files         */
/*            generated by constructs-to-c. DR0861           */
/*                                                           */                                      
/*      6.24: The DescribeClass macros were incorrectly      */
/*            defined. DR0862                                */
/*                                                           */
/*            Added allowed-classes slot facet.              */
/*                                                           */
/*            Renamed BOOLEAN macro type to intBool.         */
/*                                                           */
/*      6.30: Added EnvSlotDefaultP function.                */
/*                                                           */
/*            Borland C (IBM_TBC) and Metrowerks CodeWarrior */
/*            (MAC_MCW, IBM_MCW) are no longer supported.    */
/*                                                           */
/*            Used gensprintf and genstrcat instead of       */
/*            sprintf and strcat.                            */
/*                                                           */
/*            Added const qualifiers to remove C++           */
/*            deprecation warnings.                          */
/*                                                           */
/*            Converted API macros to function calls.        */
/*                                                           */
/*************************************************************/

#ifndef _H_classexm

#pragma once

#define _H_classexm

#if DEBUGGING_FUNCTIONS

   void                           BrowseClassesCommand(void *);
   void                           EnvBrowseClasses(void *,const char *,void *);
   void                           DescribeClassCommand(void *);
   void                           EnvDescribeClass(void *,const char *,void *);

#endif /* DEBUGGING_FUNCTIONS */

   const char                    *GetCreateAccessorString(void *);
   void                          *GetDefclassModuleCommand(void *);
   bool                           SuperclassPCommand(void *);
   bool                           EnvSuperclassP(void *,void *,void *);
   bool                           SubclassPCommand(void *);
   bool                           EnvSubclassP(void *,void *,void *);
   bool                           SlotExistPCommand(void *);
   bool                           EnvSlotExistP(void *,void *,const char *,bool);
   bool                           MessageHandlerExistPCommand(void *);
   bool                           SlotWritablePCommand(void *);
   bool                           EnvSlotWritableP(void *,void *,const char *);
   bool                           SlotInitablePCommand(void *);
   bool                           EnvSlotInitableP(void *,void *,const char *);
   bool                           SlotPublicPCommand(void *);
   bool                           EnvSlotPublicP(void *,void *,const char *);
   bool                           SlotDirectAccessPCommand(void *);
   bool                           EnvSlotDirectAccessP(void *,void *,const char *);
   void                           SlotDefaultValueCommand(void *,DATA_OBJECT_PTR);
   bool                           EnvSlotDefaultValue(void *,void *,const char *,DATA_OBJECT_PTR);
   bool                           ClassExistPCommand(void *);
   int                            EnvSlotDefaultP(void *,void *,const char *);
  
#endif /* _H_classexm */
