/*  Copyright 2004-2005 Theo Berkau
    Copyright 2005 Joost Peters

    This file is part of Yabause.

    Yabause is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Yabause is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Yabause; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <windows.h>
#include <stdio.h>
#include "cd.h"

//////////////////////////////////////////////////////////////////////////////

HANDLE hCDROM;
SCSI_PASS_THROUGH_DIRECT sptd;
HINSTANCE aspidll; 
unsigned char scsiAdapterNumber;
unsigned char lun;
unsigned char targetId;

CDInterface SPTICD = {
CDCORE_SPTI,
"Windows SPTI Driver",
SPTICDInit,
SPTICDDeInit,
SPTICDGetStatus,
SPTICDReadTOC,
SPTICDReadSectorFAD
};

CDInterface ASPICD = {
CDCORE_ASPI,
"Windows ASPI Driver",
ASPICDInit,
ASPICDDeInit,
ASPICDGetStatus,
ASPICDReadTOC,
ASPICDReadSectorFAD
};

//////////////////////////////////////////////////////////////////////////////
// SPTI Interface
//////////////////////////////////////////////////////////////////////////////

int SPTICDInit(const char *cdrom_name) {
   char pipe_name[7];

   sprintf(pipe_name, "\\\\.\\?:");
   pipe_name[4] = cdrom_name[0];

   if ((hCDROM = CreateFile(pipe_name, GENERIC_READ | GENERIC_WRITE,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                            NULL)) == INVALID_HANDLE_VALUE)
   {
      return -1;
   }

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

int SPTICDDeInit() {
   CloseHandle(hCDROM);

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

int SPTICDGetStatus() {
   // This function is called periodically to see what the status of the
   // drive is.
   //
   // Should return one of the following values:
   // 0 - CD Present, disc spinning
   // 1 - CD Present, disc not spinning
   // 2 - CD not present
   // 3 - Tray open
   //
   // If you really don't want to bother too much with this function, just
   // return status 0. Though it is kind of nice when the bios's cd player,
   // etc. recognizes when you've ejected the tray and popped in another disc.
   DWORD dwBytesReturned;
   unsigned char statusbuf[8];
   BOOL success;

   sptd.Length=sizeof(sptd);
   sptd.PathId=0;   //
   sptd.TargetId=0; // Don't need these, they're automatically generated
   sptd.Lun=0;      //
   sptd.CdbLength=12;
   sptd.SenseInfoLength=0; // No sense data
   sptd.DataIn=SCSI_IOCTL_DATA_IN; 
   sptd.TimeOutValue=60; // may need to be changed
   sptd.DataBuffer=(PVOID)&(statusbuf);
   sptd.SenseInfoOffset=0;
   sptd.DataTransferLength=8; // may need to change this

   sptd.Cdb[0]=0xBD; // CDB12 code
   sptd.Cdb[1]=0; // Reserved
   sptd.Cdb[2]=0; // Reserved
   sptd.Cdb[3]=0; // Reserved
   sptd.Cdb[4]=0; // Reserved
   sptd.Cdb[5]=0; // Reserved
   sptd.Cdb[6]=0; // Reserved
   sptd.Cdb[7]=0; // Reserved
   sptd.Cdb[8]=0; // Allocation Length(byte 1)
   sptd.Cdb[9]=8; // Allocation Length(byte 2) (may have to change this)
   sptd.Cdb[10]=0; // Reserved
   sptd.Cdb[11]=0; // Control
   sptd.Cdb[12]=0;
   sptd.Cdb[13]=0;
   sptd.Cdb[14]=0;
   sptd.Cdb[15]=0;

   success=DeviceIoControl(hCDROM, IOCTL_SCSI_PASS_THROUGH_DIRECT,
                           (PVOID)&sptd, (DWORD)sizeof(SCSI_PASS_THROUGH_DIRECT),
                           NULL, 0, &dwBytesReturned, NULL);

   if (success)
   {
      // Figure out drive status

      // Is door open?
      if (statusbuf[1] & 0x10) return 3;

      // Ok, so the door is closed, now is there a disc there?
      success = DeviceIoControl(hCDROM, IOCTL_STORAGE_CHECK_VERIFY,
                                NULL, 0, NULL, 0, &dwBytesReturned, NULL);
      if (!success)
         return 2;

      return 0;
   }

   return 2;
}

//////////////////////////////////////////////////////////////////////////////

#define MSF_TO_FAD(m,s,f) ((m * 4500) + (s * 75) + f)

s32 SPTICDReadTOC(u32 *TOC) {
//   FILE *debugfp;
   CDROM_TOC ctTOC;
   DWORD dwNotUsed;
   int i;

   if (hCDROM != INVALID_HANDLE_VALUE)
   {
      memset(TOC, 0xFF, 0xCC * 2);
      memset(&ctTOC, 0xFF, sizeof(CDROM_TOC));

      if (DeviceIoControl (hCDROM, IOCTL_CDROM_READ_TOC,
                           NULL, 0, &ctTOC, sizeof(ctTOC),
                           &dwNotUsed, NULL) == 0)
      {
         return 0;
      }

      // convert TOC to saturn format
      for (i = 0; i < ctTOC.LastTrack; i++)
      {      
         TOC[i] = (ctTOC.TrackData[i].Control << 28) |
                  (ctTOC.TrackData[i].Adr << 24) |
                   MSF_TO_FAD(ctTOC.TrackData[i].Address[1], ctTOC.TrackData[i].Address[2], ctTOC.TrackData[i].Address[3]);
      }

      // Do First, Last, and Lead out sections here

      TOC[99] = (ctTOC.TrackData[0].Control << 28) |
                (ctTOC.TrackData[0].Adr << 24) |
                (ctTOC.FirstTrack << 16);

      TOC[100] = (ctTOC.TrackData[ctTOC.LastTrack - 1].Control << 28) |
                 (ctTOC.TrackData[ctTOC.LastTrack - 1].Adr << 24) |
                 (ctTOC.LastTrack << 16);

      TOC[101] = (ctTOC.TrackData[ctTOC.LastTrack].Control << 28) |
                 (ctTOC.TrackData[ctTOC.LastTrack].Adr << 24) |
                  MSF_TO_FAD(ctTOC.TrackData[ctTOC.LastTrack].Address[1], ctTOC.TrackData[ctTOC.LastTrack].Address[2], ctTOC.TrackData[ctTOC.LastTrack].Address[3]);

//      debugfp = fopen("cot2toc.bin", "wb");
//      fwrite((void *)TOC, 1, 0xCC * 2, debugfp);
//      fclose(debugfp);

      return (0xCC * 2);
   }

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

int SPTICDReadSectorFAD(u32 FAD, void *buffer) {
   // This function is supposed to read exactly 1 -RAW- 2352-byte sector at
   // the specified FAD address to buffer. Should return true if successful,
   // false if there was an error.
   //
   // Special Note: To convert from FAD to LBA/LSN, minus 150.
   //
   // The whole process needed to be changed since I need more control over
   // sector detection, etc. Not to mention it means less work for the porter
   // since they only have to implement raw sector reading as opposed to
   // implementing mode 1, mode 2 form1/form2, -and- raw sector reading.

   DWORD dwBytesReturned;
   BOOL success;

   sptd.Length=sizeof(sptd);
   sptd.PathId=0;   //
   sptd.TargetId=0; // Don't need these, they're automatically generated
   sptd.Lun=0;      //
   sptd.CdbLength=12;
   sptd.SenseInfoLength=0; // No sense data
   sptd.DataIn=SCSI_IOCTL_DATA_IN;
   sptd.TimeOutValue=60; // may need to be changed
   sptd.DataBuffer=(PVOID)buffer;
   sptd.SenseInfoOffset=0;
   sptd.DataTransferLength=2352; 

   sptd.Cdb[0]=0xBE; // CDB12 code
   sptd.Cdb[1]=0; // Sector Type, RELADR
   FAD -= 150;
   sptd.Cdb[2]=(unsigned char)((FAD & 0xFF000000) >> 24); // lba(byte 1)
   sptd.Cdb[3]=(unsigned char)((FAD & 0x00FF0000) >> 16); // lba(byte 2)
   sptd.Cdb[4]=(unsigned char)((FAD & 0x0000FF00) >> 8);  // lba(byte 3)
   sptd.Cdb[5]=(unsigned char)(FAD & 0x000000FF);         // lba(byte 4)
   sptd.Cdb[6]=0; // number of sectors(byte 1)
   sptd.Cdb[7]=0; // number of sectors(byte 2)
   sptd.Cdb[8]=1; // number of sectors(byte 3)
   sptd.Cdb[9]=0xF8; // Sync + All Headers + User data + EDC/ECC
   sptd.Cdb[10]=0;
   sptd.Cdb[11]=0;
   sptd.Cdb[12]=0;
   sptd.Cdb[13]=0;
   sptd.Cdb[14]=0;
   sptd.Cdb[15]=0;

   success=DeviceIoControl(hCDROM,
                           IOCTL_SCSI_PASS_THROUGH_DIRECT,
                           (PVOID)&sptd, (DWORD)sizeof(SCSI_PASS_THROUGH_DIRECT),
                           NULL, 0,
                           &dwBytesReturned,
                           NULL);

   if (!success)
   {
      return 0;
   }

   return 1;
}

//////////////////////////////////////////////////////////////////////////////
// ASPI Interface
//////////////////////////////////////////////////////////////////////////////

int ASPICDInit(const char *cdrom_name) {
   OSVERSIONINFO osvi;
   BOOL result;
//   SRB_GDEVBlock devtypecmd;

   if (!(aspidll = LoadLibrary("WNASPI32.DLL")))
      return -1;

   fprintf(stderr, "ASPI dll loaded successfully\n");

/*
   GetASPI32SupportInfo = (DWORD (*)(void))GetProcAddress(aspidll, "GetASPI32SupportInfo");
   SendASPI32Command = (DWORD (*)(LPSRB))GetProcAddress(aspidll, "SendASPI32Command");

   if (GetASPI32SupportInfo == NULL || SendASPI32Command == NULL)
   {
      FreeLibrary(aspidll);
      aspidll = NULL;
      return -1;
   }
*/

   fprintf(stderr, "ASPI functions retrieved succesfully\n");

   // Figure out which OS we're using
   ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
   osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
   result = GetVersionEx(&osvi);

   // Use IOCTL_SCSI_GET_ADDRESS and registry on NT based platforms
   if (result == TRUE && osvi.dwPlatformId == VER_PLATFORM_WIN32_NT)
   {
      HANDLE hCDROM;
      char pipe_name[7];
      BOOL success;
      SCSI_ADDRESS saBuf;
      DWORD dwBytesReturned;

      fprintf(stderr, "Win NT platform detected\n");

      sprintf(pipe_name, "\\\\.\\?:");
      pipe_name[4] = cdrom_name[0];

      if ((hCDROM = CreateFile(pipe_name,
                          GENERIC_READ | GENERIC_WRITE,
                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                          NULL,
                          OPEN_EXISTING,
                          FILE_ATTRIBUTE_NORMAL,
                          NULL)) == INVALID_HANDLE_VALUE)
      {
         FreeLibrary(aspidll);
         aspidll = NULL;
         return -1;
      }

      success=DeviceIoControl(hCDROM,
                              IOCTL_SCSI_GET_ADDRESS,
                              NULL, 0,
                              (LPVOID)&saBuf, sizeof(SCSI_ADDRESS),
                              &dwBytesReturned, NULL);

      if (success)
      {
         fprintf(stderr, "IOCTL_SCSI_GET_ADDRESS successful: SCSI adapter number = %d, LUN = %d, Target ID = %d\n", saBuf.PortNumber, saBuf.Lun, saBuf.TargetId);

         scsiAdapterNumber = saBuf.PortNumber;
         lun = saBuf.Lun;
         targetId = saBuf.TargetId;

/*
//         if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,
//                         "Hardware\DeviceMap\SCSI",
//                         0,
//                         KEY_READ,
//                         &hCDROMKey) != ERROR_SUCCESS)
         {
//          return ??;
         }

//         retCode = RegQueryInfoKey(hCDROMKey, achClass, &cchClassName, NULL,
//                                   &cSubKeys, &cbMaxSubKey, &cchMaxClass,
//                                   &cValues, &cchMaxValue, &cbMaxValueData,
//                                   &cbSecurityDescriptor, &ftLastWriteTime);

         for (int i = 0; i < cSubKeys; i++)
         {
            cbName = MAX_KEY_LENGTH;
            retCode = RegEnumKeyEx(hKey, i, achKey, &cbName, NULL, NULL, NULL,
                                   &ftLastWriteTime);
            // Grab HardwareKey subkey
            if (retCode == ERROR_SUCCESS)
         }
*/
         CloseHandle(hCDROM);
      }
      else
      {
         fprintf(stderr, "IOCTL_SCSI_GET_ADDRESS failed\n");
      }
   }
   else
   {      
      fprintf(stderr, "non-Win NT platform detected\n");
/*
      // Assume it's a Win9x platform, use registry only
      HKEY hCDROMkey;

      if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                      "Config Manager\\Enum",
                      0,
                      KEY_READ,
                      &hCDROMKey) != ERROR_SUCCESS)
      {
//         return ??;
      }

//      retCode = RegQueryInfoKey(hCDROMKey, achClass, &cchClassName, NULL,
//                                &cSubKeys, &cbMaxSubKey, &cchMaxClass,
//                                &cValues, &cchMaxValue, &cbMaxValueData,
//                                &cbSecurityDescriptor, &ftLastWriteTime);

      for (int i = 0; i < cSubKeys; i++)
      {
         cbName = MAX_KEY_LENGTH;
         retCode = RegEnumKeyEx(hKey, i, achKey, &cbName, NULL, NULL, NULL,
                                &ftLastWriteTime);
         // Grab HardwareKey subkey
         if (retCode == ERROR_SUCCESS)

      }
*/
   }

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

int ASPICDDeInit() {
   if (aspidll)
      FreeLibrary(aspidll);

   fprintf(stderr, "ASPI dll freed successfully\n");

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

int ASPICDGetStatus() {
   // This function is called periodically to see what the status of the
   // drive is.
   //
   // Should return one of the following values:
   // 0 - CD Present, disc spinning
   // 1 - CD Present, disc not spinning
   // 2 - CD not present
   // 3 - Tray open
   //
   // If you really don't want to bother too much with this function, just
   // return status 0. Though it is kind of nice when the bios's cd player,
   // etc. recognizes when you've ejected the tray and popped in another disc.
   return 2;
}

//////////////////////////////////////////////////////////////////////////////

s32 ASPICDReadTOC(u32 *TOC) {
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

int ASPICDReadSectorFAD(u32 FAD, void *buffer) {
   // This function is supposed to read exactly 1 -RAW- 2352-byte sector at
   // the specified FAD address to buffer. Should return true if successful,
   // false if there was an error.
   //
   // Special Note: To convert from FAD to LBA/LSN, minus 150.
   //
   // The whole process needed to be changed since I need more control over
   // sector detection, etc. Not to mention it means less work for the porter
   // since they only have to implement raw sector reading as opposed to
   // implementing mode 1, mode 2 form1/form2, -and- raw sector reading.
   return 1;
}

//////////////////////////////////////////////////////////////////////////////
