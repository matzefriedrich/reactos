/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id: bug.c,v 1.47.2.1 2004/12/13 09:39:11 hyperion Exp $
 *
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/bug.c
 * PURPOSE:         Graceful system shutdown if a bug is detected
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * PORTABILITY:     Unchecked
 * UPDATE HISTORY:
 *                  Created 22/05/98
 *                  Phillip Susi: 12/8/99: Minor fix
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#include <ntos/bootvid.h>
#include <internal/debug.h>
#include "../../hal/halx86/include/hal.h"

/* GLOBALS ******************************************************************/

static LIST_ENTRY BugcheckCallbackListHead = {NULL,NULL};
static ULONG InBugCheck;

/* FUNCTIONS *****************************************************************/

VOID INIT_FUNCTION
KeInitializeBugCheck(VOID)
{
  InitializeListHead(&BugcheckCallbackListHead);
  InBugCheck = 0;
}

/*
 * @implemented
 */
BOOLEAN STDCALL
KeDeregisterBugCheckCallback(PKBUGCHECK_CALLBACK_RECORD CallbackRecord)
{
	/* Check the Current State */
	if (CallbackRecord->State == BufferInserted) {
		CallbackRecord->State = BufferEmpty;
		RemoveEntryList(&CallbackRecord->Entry);
		return TRUE;
	}
	
	/* The callback wasn't registered */
	return FALSE;
}

/*
 * @implemented
 */
BOOLEAN STDCALL
KeRegisterBugCheckCallback(PKBUGCHECK_CALLBACK_RECORD CallbackRecord,
			   PKBUGCHECK_CALLBACK_ROUTINE	CallbackRoutine,
			   PVOID Buffer,
			   ULONG Length,
			   PUCHAR Component)
{

	/* Check the Current State first so we don't double-register */
	if (CallbackRecord->State == BufferEmpty) {
		CallbackRecord->Length = Length;
		CallbackRecord->Buffer = Buffer;
		CallbackRecord->Component = Component;
		CallbackRecord->CallbackRoutine = CallbackRoutine;
		CallbackRecord->State = BufferInserted;
		InsertTailList(&BugcheckCallbackListHead, &CallbackRecord->Entry);
		
		return TRUE;
	}
  
	/* The Callback was already registered */
	return(FALSE);
}

VOID STDCALL
KeBugCheckWithTf(ULONG BugCheckCode, 	     
		 ULONG BugCheckParameter1,
		 ULONG BugCheckParameter2,
		 ULONG BugCheckParameter3,
		 ULONG BugCheckParameter4,
		 PKTRAP_FRAME Tf)
{
  PRTL_MESSAGE_RESOURCE_ENTRY Message;
  NTSTATUS Status;
  ULONG Mask;
  KIRQL OldIrql;

  /* Make sure we're switching back to the blue screen and print messages on it */
  HalReleaseDisplayOwnership();
  if (0 == (KdDebugState & KD_DEBUG_GDB))
    {
      KdDebugState |= KD_DEBUG_SCREEN;
    }

  Ke386DisableInterrupts();
  DebugLogDumpMessages();

  if (KeGetCurrentIrql() < DISPATCH_LEVEL)
    {
      KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
    }
  DbgPrint("Bug detected (code %x param %x %x %x %x)\n",
	   BugCheckCode,
	   BugCheckParameter1,
	   BugCheckParameter2,
	   BugCheckParameter3,
	   BugCheckParameter4);

  Status = RtlFindMessage((PVOID)KERNEL_BASE, //0xC0000000,
			  11, //RT_MESSAGETABLE,
			  0x09, //0x409,
			  BugCheckCode,
			  &Message);
  if (NT_SUCCESS(Status))
    {
      if (Message->Flags == 0)
	DbgPrint("  %s\n", Message->Text);
      else
	DbgPrint("  %S\n", (PWSTR)Message->Text);
    }
  else
    {
      DbgPrint("  No message text found!\n\n");
    }
  Mask = 1 << KeGetCurrentProcessorNumber();
  if (InBugCheck & Mask)
    {
#ifdef MP
      DbgPrint("Recursive bug check on CPU%d, halting now\n", KeGetCurrentProcessorNumber());
      /*
       * FIXME:
       *   Send an ipi to all other processors which halt them too.
       */
#else
      DbgPrint("Recursive bug check halting now\n");
#endif
      Ke386HaltProcessor();
    }
  /* 
   * FIXME:
   *   Use InterlockedOr or InterlockedBitSet.
   */
  InBugCheck |= Mask;
  if (Tf != NULL)
    {
      KiDumpTrapFrame(Tf, BugCheckParameter1, BugCheckParameter2);
    }
  else
    {
#if defined(__GNUC__)
      KeDumpStackFrames((PULONG)__builtin_frame_address(0));
#elif defined(_MSC_VER)
      __asm push ebp
      __asm call KeDumpStackFrames
      __asm add esp, 4
#else
#error Unknown compiler for inline assembler
#endif
    }
  MmDumpToPagingFile(BugCheckCode, BugCheckParameter1, 
		     BugCheckParameter2, BugCheckParameter3,
		     BugCheckParameter4, Tf);

  if (KdDebuggerEnabled)
    {
      Ke386EnableInterrupts();
      DbgBreakPointNoBugCheck();
      Ke386DisableInterrupts();
    }

  for (;;)
    {
      /*
       * FIXME:
       *   Send an ipi to all other processors which halt them too.
       */
      Ke386HaltProcessor();
    }
}

/*
 * @implemented
 */
VOID STDCALL
KeBugCheckEx(ULONG BugCheckCode,
	     ULONG BugCheckParameter1,
	     ULONG BugCheckParameter2,
	     ULONG BugCheckParameter3,
	     ULONG BugCheckParameter4)
/*
 * FUNCTION: Brings the system down in a controlled manner when an 
 * inconsistency that might otherwise cause corruption has been detected
 * ARGUMENTS:
 *           BugCheckCode = Specifies the reason for the bug check
 *           BugCheckParameter[1-4] = Additional information about bug
 * RETURNS: Doesn't
 */
{
  KeBugCheckWithTf(BugCheckCode, BugCheckParameter1, BugCheckParameter2,
		   BugCheckParameter3, BugCheckParameter4, NULL);
}

/*
 * @implemented
 */
VOID STDCALL
KeBugCheck(ULONG BugCheckCode)
/*
 * FUNCTION: Brings the system down in a controlled manner when an 
 * inconsistency that might otherwise cause corruption has been detected
 * ARGUMENTS:
 *           BugCheckCode = Specifies the reason for the bug check
 * RETURNS: Doesn't
 */
{
  KeBugCheckEx(BugCheckCode, 0, 0, 0, 0);
}

/* EOF */
