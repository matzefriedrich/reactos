/*
 *  ReactOS W32 Subsystem
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003 ReactOS Team
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
/* $Id: print.c,v 1.23.8.1 2004/12/13 09:39:22 hyperion Exp $ */
#include <w32k.h>

INT
STDCALL
NtGdiAbortDoc(HDC  hDC)
{
  UNIMPLEMENTED;
  return 0;
}

INT
STDCALL
NtGdiEndDoc(HDC  hDC)
{
  UNIMPLEMENTED;
  return 0;
}

INT
STDCALL
NtGdiEndPage(HDC  hDC)
{
  UNIMPLEMENTED;
  return 0;
}

INT
FASTCALL
IntGdiEscape(PDC    dc,
             INT    Escape,
             INT    InSize,
             LPCSTR InData,
             LPVOID OutData)
{
  UNIMPLEMENTED;
  return 0;
}

INT
STDCALL
NtGdiEscape(HDC  hDC,
                INT  Escape,
                INT  InSize,
                LPCSTR  InData,
                LPVOID  OutData)
{
  PDC dc;
  INT ret;

  dc = DC_LockDc(hDC);
  if (dc == NULL)
  {
    SetLastWin32Error(ERROR_INVALID_HANDLE);
    return 0;
  }

  /* TODO FIXME - don't pass umode buffer to an Int function */
  ret = IntGdiEscape(dc, Escape, InSize, InData, OutData);

  DC_UnlockDc( hDC );
  return ret;
}

INT
STDCALL
IntEngExtEscape(
   SURFOBJ *Surface,
   INT      Escape,
   INT      InSize,
   LPVOID   InData,
   INT      OutSize,
   LPVOID   OutData)
{
   if (Escape == 0x1101)
      return 0;

   UNIMPLEMENTED;
   return -1;
}

INT
STDCALL
IntGdiExtEscape(
   PDC    dc,
   INT    Escape,
   INT    InSize,
   LPCSTR InData,
   INT    OutSize,
   LPSTR  OutData)
{
   BITMAPOBJ *BitmapObj = BITMAPOBJ_LockBitmap(dc->w.hBitmap);
   INT Result;

   /* FIXME - Handle BitmapObj == NULL !!!!!! */

   if ( NULL == dc->DriverFunctions.Escape )
   {
      Result = IntEngExtEscape(
         &BitmapObj->SurfObj,
         Escape,
         InSize,
         (PVOID)InData,
         OutSize,
         (PVOID)OutData);
   }
   else
   {
      Result = dc->DriverFunctions.Escape(
         &BitmapObj->SurfObj,
         Escape,
         InSize,
         (PVOID)InData,
         OutSize,
         (PVOID)OutData );
   }
   BITMAPOBJ_UnlockBitmap(dc->w.hBitmap);

   return Result;
}

INT
STDCALL
NtGdiExtEscape(
   HDC    hDC,
   INT    Escape,
   INT    InSize,
   LPCSTR UnsafeInData,
   INT    OutSize,
   LPSTR  UnsafeOutData)
{
   PDC      pDC = DC_LockDc(hDC);
   LPVOID   SafeInData = NULL;
   LPVOID   SafeOutData = NULL;
   NTSTATUS Status;
   INT      Result;

   if ( pDC == NULL )
   {
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      return -1;
   }

   if ( InSize && UnsafeInData )
   {
      SafeInData = ExAllocatePoolWithTag ( PagedPool, InSize, TAG_PRINT );
      if ( !SafeInData )
      {
         DC_UnlockDc(hDC);
         SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
         return -1;
      }
      Status = MmCopyFromCaller ( SafeInData, UnsafeInData, InSize );
      if ( !NT_SUCCESS(Status) )
      {
         ExFreePool ( SafeInData );
         DC_UnlockDc(hDC);
         SetLastNtError(Status);
         return -1;
      }
   }

   if ( OutSize && UnsafeOutData )
   {
      SafeOutData = ExAllocatePoolWithTag ( PagedPool, OutSize, TAG_PRINT );
      if ( !SafeOutData )
      {
         if ( SafeInData )
            ExFreePool ( SafeInData );
         DC_UnlockDc(hDC);
         SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
         return -1;
      }
   }

   Result = IntGdiExtEscape ( pDC, Escape, InSize, SafeInData, OutSize, SafeOutData );

   DC_UnlockDc(hDC);

   if ( SafeInData )
      ExFreePool ( SafeInData );

   if ( SafeOutData )
   {
      Status = MmCopyToCaller ( UnsafeOutData, SafeOutData, OutSize );
      ExFreePool ( SafeOutData );
      if ( !NT_SUCCESS(Status) )
      {
         SetLastNtError(Status);
         return -1;
      }
   }

   return Result;
}

INT
STDCALL
NtGdiSetAbortProc(HDC  hDC,
                      ABORTPROC  AbortProc)
{
  UNIMPLEMENTED;
  return 0;
}

INT
STDCALL
NtGdiStartDoc(HDC  hDC,
                  CONST LPDOCINFOW  di)
{
  UNIMPLEMENTED;
  return 0;
}

INT
STDCALL
NtGdiStartPage(HDC  hDC)
{
  UNIMPLEMENTED;
  return 0;
}
/* EOF */
