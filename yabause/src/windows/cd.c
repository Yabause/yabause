#include <windows.h>
#include <winioctl.h>
#include "ntddcdrm.h"
#include "ntddscsi.h"
#include <stdio.h>
//#include "main.h"

HANDLE hCDROM;
SCSI_PASS_THROUGH_DIRECT sptd;

int CDInit(char *cdrom_name)
{
   char pipe_name[7];

   sprintf(pipe_name, "\\\\.\\?:\0");
   pipe_name[4] = cdrom_name[0];

   if ((hCDROM = CreateFile(pipe_name,
                       GENERIC_READ | GENERIC_WRITE,
                       FILE_SHARE_READ | FILE_SHARE_WRITE,
                       NULL,
                       OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL)) == INVALID_HANDLE_VALUE)
   {
      // try loading aspi here

      return -1;
   }

   return 0;
}

int CDDeInit()
{
   CloseHandle(hCDROM);

   return 0;
}


bool CDIsCDPresent()
{
   DWORD dwBytesReturned;
   BOOL success;

   success = DeviceIoControl(hCDROM, IOCTL_STORAGE_CHECK_VERIFY,
                             NULL, 0, NULL, 0, &dwBytesReturned, NULL);
   return success;
}

#define MSF_TO_FAD(m,s,f) ((m * 4500) + (s * 75) + f)

long CDReadToc(unsigned long *TOC)
{
   BOOL success;
   FILE *debugfp;
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

      printf("cd:\tToc info: First Track = %d Last Track = %d\n", ctTOC.FirstTrack, ctTOC.LastTrack);

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

//      if ((debugfp = fopen("test.out", "wb")) != NULL)
//      {
//        fwrite((void *)TOC, 1, 2 * 0xCC, debugfp);
//        fclose(debugfp);
//      }

      return (0xCC * 2);
   }

   return 0;
}

unsigned long CDReadSector(unsigned long lba, unsigned long size, void *buffer)
{
   BOOL success;
   DWORD dwBytesReturned;

   if (hCDROM != INVALID_HANDLE_VALUE)
   {
/*      char tempstr[128];

      sprintf(tempstr, "type = %d lba = %d size = %d\n", type, lba, size);
      MessageBox (NULL, tempstr, "Notice",
                  MB_OK | MB_ICONINFORMATION);

      sptd.Length=sizeof(sptd);
      sptd.PathId=0;   // Don't need these, they're automatically generated
      sptd.TargetId=0;
      sptd.Lun=0;     
      sptd.CdbLength=12; 
      sptd.SenseInfoLength=0; // No sense data
      sptd.DataIn=SCSI_IOCTL_DATA_IN;
      sptd.TimeOutValue=60;  // may need to be changed
      sptd.DataBuffer=(PVOID)&(buffer);
      sptd.SenseInfoOffset=0;
      sptd.DataTransferLength=size; 

      sptd.Cdb[0]=0xBE; // CDB12 code
      // fix me
      sptd.Cdb[1]=0;
      sptd.Cdb[2]=(unsigned char)((lba & 0xFF000000) >> 24); // lba(byte 1)
      sptd.Cdb[3]=(unsigned char)((lba & 0x00FF0000) >> 16); // lba(byte 2)
      sptd.Cdb[4]=(unsigned char)((lba & 0x0000FF00) >> 8);  // lba(byte 3)
      sptd.Cdb[5]=(unsigned char)(lba & 0x000000FF);         // lba(byte 4)
      // fix me
      sptd.Cdb[6]=0; // number of sectors(byte 1)
      sptd.Cdb[7]=0; // number of sectors(byte 2)
      sptd.Cdb[8]=1; // number of sectors(byte 3)
      // fix me
      sptd.Cdb[9]=0x10;  // user data only
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
*/

      SetFilePointer(hCDROM, lba * 2048, 0, FILE_BEGIN);
      success = ReadFile(hCDROM, buffer, size, &dwBytesReturned, NULL);

      return dwBytesReturned;
   }

   return 0;
}
