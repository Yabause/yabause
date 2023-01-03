/*  
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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/

/*! \file movie.c
    \brief Movie recording functions.
*/

#include "peripheral.h"
#include "scsp.h"
#include "movie.h"
#include "cs2.h"
#include "vdp2.h"  // for DisplayMessage() prototype
#include "yabause.h"
#include "error.h"

int RecordingFileOpened;
int PlaybackFileOpened;

struct MovieStruct Movie;

char MovieStatus[40];
int movieLoaded = 0;	//Boolean value, 1 if a movie is playing or recording

//Counting
int framecounter;
int lagframecounter;
int LagFrameFlag;
int FrameAdvanceVariable=0;

int headersize=512;

//////////////////////////////////////////////////////////////////////////////

static void ReadHeader(FILE* fp) {
   size_t num_read = 0;

	fseek(fp, 0, SEEK_SET);

	fseek(fp, 172, SEEK_SET);
   num_read = fread(&Movie.Rerecords, sizeof(Movie.Rerecords), 1, fp);

	fseek(fp, headersize, SEEK_SET);
}

//////////////////////////////////////////////////////////////////////////////

static void WriteHeader(FILE* fp) {

	fseek(fp, 0, SEEK_SET);

	fwrite("YMV", sizeof("YMV"), 1, fp);
	fwrite(VERSION, sizeof(VERSION), 1, fp);
	fwrite(cdip->cdinfo, sizeof(cdip->cdinfo), 1, fp);
	fwrite(cdip->itemnum, sizeof(cdip->itemnum), 1, fp);
	fwrite(cdip->version, sizeof(cdip->version), 1, fp);
	fwrite(cdip->date, sizeof(cdip->date), 1, fp);
	fwrite(cdip->gamename, sizeof(cdip->gamename), 1, fp);
	fwrite(cdip->region, sizeof(cdip->region), 1, fp);
	fwrite(&Movie.Rerecords, sizeof(Movie.Rerecords), 1, fp);
	fwrite(&yabsys.emulatebios, sizeof(yabsys.emulatebios), 1, fp);
	fwrite(&yabsys.IsPal, sizeof(yabsys.IsPal), 1, fp);

	fseek(fp, headersize, SEEK_SET);
}

//////////////////////////////////////////////////////////////////////////////

static void ClearInput(void) {

	//do something....
}

//////////////////////////////////////////////////////////////////////////////

const char* Buttons[8] = {"B", "C", "A", "S", "U", "D", "R", "L"};
const char* Spaces[8]  = {" ", " ", " ", " ", " ", " ", " ", " "};
const char* Buttons2[8] = {"", "", "", "L", "Z", "Y", "X", "R"};
const char* Spaces2[8]  = {"", "", "", " ", " ", " ", " ", " "};

char str[40];
char InputDisplayString[40];

static void SetInputDisplayCharacters(void) {

	int x;

	strcpy(str, "");

	for (x = 0; x < 8; x++) {

		if(PORTDATA1.data[2] & (1 << x)) {
         size_t spaces_len = strlen(Spaces[x]);
         if (spaces_len >= 40)
            return;
			strcat(str, Spaces[x]);	
		}
      else
      {
         size_t buttons_len = strlen(Buttons[x]);
         if (buttons_len >= 40)
            return;
         strcat(str, Buttons[x]);
      }

	}

	for (x = 0; x < 8; x++) {

		if(PORTDATA1.data[3] & (1 << x)) {
         size_t spaces2_len = strlen(Spaces2[x]);
         if (spaces2_len >= 40)
            return;
			strcat(str, Spaces2[x]);	
		}
      else
      {
         size_t buttons2_len = strlen(Buttons2[x]);
         if (buttons2_len >= 40)
            return;
         strcat(str, Buttons2[x]);
      }

	}

	strcpy(InputDisplayString, str);
}

//////////////////////////////////////////////////////////////////////////////

static void IncrementLagAndFrameCounter(void)
{
	if(LagFrameFlag == 1)
		lagframecounter++;

	framecounter++;
}

//////////////////////////////////////////////////////////////////////////////

int framelength=16;

void DoMovie(void) {

	int x;
   size_t num_read = 0;

	if (Movie.Status == 0)
		return;

	IncrementLagAndFrameCounter();
	LagFrameFlag=1;
	SetInputDisplayCharacters();

	//Read/Write Controller Data
	if(Movie.Status == Recording) {
		for (x = 0; x < 8; x++) {
			fwrite(&PORTDATA1.data[x], 1, 1, Movie.fp);
		}
		for (x = 0; x < 8; x++) {
			fwrite(&PORTDATA2.data[x], 1, 1, Movie.fp);
		}
	}

	if(Movie.Status == Playback) {
		for (x = 0; x < 8; x++) {
         num_read = fread(&PORTDATA1.data[x], 1, 1, Movie.fp);
		}
		for (x = 0; x < 8; x++) {
         num_read = fread(&PORTDATA2.data[x], 1, 1, Movie.fp);
		}

		//if we get to the end of the movie
		if(((ftell(Movie.fp)-headersize)/framelength) >= Movie.Frames) {
			fclose(Movie.fp);
			PlaybackFileOpened=0;
			Movie.Status = Stopped;
			ClearInput();
			strcpy(MovieStatus, "Playback Stopped");
		}
	}

	//Stop Recording/Playback
	if(Movie.Status != Recording && RecordingFileOpened) {
		fclose(Movie.fp);
		RecordingFileOpened=0;
		Movie.Status = Stopped;
		strcpy(MovieStatus, "Recording Stopped");
	}

	if(Movie.Status != Playback && PlaybackFileOpened && Movie.ReadOnly != 0) {
		fclose(Movie.fp);
		PlaybackFileOpened=0;
		Movie.Status = Stopped;
		strcpy(MovieStatus, "Playback Stopped");
	}
}

//////////////////////////////////////////////////////////////////////////////

void MovieLoadState(void) {


	if (Movie.ReadOnly == 1 && Movie.Status == Playback)  {
		//Movie.Status = Playback;
		fseek (Movie.fp,headersize+(framecounter * framelength),SEEK_SET);
	}

	if(Movie.Status == Recording) {
		fseek (Movie.fp,headersize+(framecounter * framelength),SEEK_SET);
		Movie.Rerecords++;
	}

	if(Movie.Status == Playback && Movie.ReadOnly == 0) {
		Movie.Status = Recording;
		RecordingFileOpened=1;
		strcpy(MovieStatus, "Recording Resumed");
		TruncateMovie(Movie);
		fseek (Movie.fp,headersize+(framecounter * framelength),SEEK_SET);
		Movie.Rerecords++;
	}
}

//////////////////////////////////////////////////////////////////////////////

void TruncateMovie(struct MovieStruct Movie) {

	//when we resume recording, shorten the movie so that there isn't 
	//potential garbage data at the end

/*//TODO
	struct MovieBufferStruct tempbuffer;
	fseek(Movie.fp,0,SEEK_SET);
	tempbuffer=ReadMovieIntoABuffer(Movie.fp);
	fclose(Movie.fp);

	//clear the file and write it again
	Movie.fp=fopen(Movie.filename,"wb");
	fwrite(tempbuffer.data,framelength,framecounter,Movie.fp);
	fclose(Movie.fp);

	Movie.fp=fopen(Movie.filename,"r+b");
*/
}

//////////////////////////////////////////////////////////////////////////////

static int MovieGetSize(FILE* fp) {
	int size;
	int fpos;

	fpos = ftell(fp);//save current pos

   if (fpos < 0)
   {
      YabSetError(YAB_ERR_OTHER, "MovieGetSize fpos is negative");
      return 0;
   }

	fseek (fp,0,SEEK_END);
	size=ftell(fp);

	Movie.Frames=(size-headersize)/ framelength;

	fseek(fp, fpos, SEEK_SET); //reset back to correct pos
	return(size);
}

//////////////////////////////////////////////////////////////////////////////

void MovieToggleReadOnly(void) {

	if(Movie.Status == Playback) {

		if(Movie.ReadOnly == 1) 
		{
			Movie.ReadOnly=0;
			DisplayMessage("Movie is now read+write.");
		}
		else 
		{
			Movie.ReadOnly=1;
			DisplayMessage("Movie is now read only.");
		}
	}
}

//////////////////////////////////////////////////////////////////////////////

void StopMovie(void) {

	if(Movie.Status == Recording && RecordingFileOpened) {
		WriteHeader(Movie.fp);
		fclose(Movie.fp);
		RecordingFileOpened=0;
		Movie.Status = Stopped;
		ClearInput();
		strcpy(MovieStatus, "Recording Stopped");
	}

	if(Movie.Status == Playback && PlaybackFileOpened && Movie.ReadOnly != 0) {
		fclose(Movie.fp);
		PlaybackFileOpened=0;
		Movie.Status = Stopped;
		ClearInput();
		strcpy(MovieStatus, "Playback Stopped");
	}
}

//////////////////////////////////////////////////////////////////////////////

int SaveMovie(const char *filename) {

	char* str=malloc(1024);

	if(Movie.Status == Playback)
		StopMovie();

	if ((Movie.fp = fopen(filename, "w+b")) == NULL)
	{
		free(str);
		return -1;
	}

	strcpy(str, filename);
	Movie.filename=str;
	RecordingFileOpened=1;
	framecounter=0;
	Movie.Status=Recording;
	strcpy(MovieStatus, "Recording Started");
	Movie.Rerecords=0;
	WriteHeader(Movie.fp);
	YabauseReset();
	return 0;
}

//////////////////////////////////////////////////////////////////////////////

int PlayMovie(const char *filename) {

	char* str=malloc(1024);

	if(Movie.Status == Recording)
		StopMovie();


	if ((Movie.fp = fopen(filename, "r+b")) == NULL)
	{
		free(str);
		return -1;
	}

	strcpy(str, filename);
	Movie.filename=str;
	PlaybackFileOpened=1;
	framecounter=0;
	Movie.ReadOnly = 1;
	Movie.Status=Playback;
	Movie.Size = MovieGetSize(Movie.fp);
	strcpy(MovieStatus, "Playback Started");
	ReadHeader(Movie.fp);
	YabauseReset();
	return 0;
}

//////////////////////////////////////////////////////////////////////////////

void SaveMovieInState(void ** stream) {

	struct MovieBufferStruct tempbuffer;

	if(Movie.Status == Recording || Movie.Status == Playback) {
		tempbuffer=ReadMovieIntoABuffer(Movie.fp);

		MemStateWrite(&tempbuffer.size, 4, 1, stream);
		MemStateWrite(tempbuffer.data, tempbuffer.size, 1, stream);

		free(tempbuffer.data);
	}
}

//////////////////////////////////////////////////////////////////////////////

void MovieReadState(const void * stream) {

	ReadMovieInState(stream);
	MovieLoadState();//file pointer and truncation

}

void ReadMovieInState(const void * stream) {

	struct MovieBufferStruct tempbuffer;
	int fpos;
   size_t num_read = 0;

	//overwrite the main movie on disk if we are recording or read+write playback
	if(Movie.Status == Recording || (Movie.Status == Playback && Movie.ReadOnly == 0)) {

		fpos=MemStateGetOffset();//where we are in the savestate

      if (fpos < 0)
      {
         YabSetError(YAB_ERR_OTHER, "ReadMovieInState fpos is negative");
         return;
      }

      MemStateRead(&tempbuffer.size, 4, 1, stream);//size
		if ((tempbuffer.data = (char *)malloc(tempbuffer.size)) == NULL)
		{
			return;
		}
      MemStateRead(tempbuffer.data, 1, tempbuffer.size, stream);//movie
		MemStateSetOffset(fpos);//reset savestate position

		rewind(Movie.fp);
		fwrite(tempbuffer.data, 1, tempbuffer.size, Movie.fp);
		rewind(Movie.fp);
	}
}

//////////////////////////////////////////////////////////////////////////////

struct MovieBufferStruct ReadMovieIntoABuffer(FILE* fp) {

	int fpos;
   struct MovieBufferStruct tempbuffer = { 0 };
   size_t num_read = 0;

	fpos = ftell(fp);//save current pos

   if (fpos < 0)
   {
      YabSetError(YAB_ERR_OTHER, "ReadMovieIntoABuffer fpos is negative");
      return tempbuffer;
   }

	fseek (fp,0,SEEK_END);
	tempbuffer.size=ftell(fp);  //get size
	rewind(fp);

	tempbuffer.data = (char*) malloc (sizeof(char)*tempbuffer.size);
   num_read = fread(tempbuffer.data, 1, tempbuffer.size, fp);

	fseek(fp, fpos, SEEK_SET); //reset back to correct pos
	return(tempbuffer);
}

//////////////////////////////////////////////////////////////////////////////

const char *MakeMovieStateName(const char *filename) {

	static char *retbuf = NULL;  // Save the pointer to avoid memory leaks
	if(Movie.Status == Recording || Movie.Status == Playback) {
		const size_t newsize = strlen(filename) + 5 + 1;
		free(retbuf);
		retbuf = malloc(newsize);
		if (!retbuf) {
			return NULL;  // out of memory
		}
		sprintf(retbuf, "%smovie", filename);
		return retbuf;
	} else {
		return filename;  // unchanged
	}

}

//////////////////////////////////////////////////////////////////////////////

//debugging only
void TestWrite(struct MovieBufferStruct tempbuffer) {

	FILE* tempbuffertest;

	tempbuffertest=fopen("rmiab.txt", "wb");

   if (!tempbuffertest)
      return;

	fwrite (tempbuffer.data, 1, tempbuffer.size, tempbuffertest);
	fclose(tempbuffertest);
}

//////////////////////////////////////////////////////////////////////////////

void PauseOrUnpause(void) {

	if(FrameAdvanceVariable == RunNormal) {
		FrameAdvanceVariable=Paused;
		ScspMuteAudio(SCSP_MUTE_SYSTEM);
	}
	else {
		FrameAdvanceVariable=RunNormal;	
		ScspUnMuteAudio(SCSP_MUTE_SYSTEM);
	}
}

//////////////////////////////////////////////////////////////////////////////

int IsMovieLoaded(void)
{
	if (RecordingFileOpened || PlaybackFileOpened)
		return 1;
	else
		return 0;
}

//////////////////////////////////////////////////////////////////////////////
