#ifndef MOVIE_H
#define MOVIE_H

#define Stopped	  1
#define Recording 2
#define Playback  3

#define RunNormal   0
#define Paused      1
#define NeedAdvance 2

void DoMovie(void);

struct MovieStruct
{
	int Status;
	FILE *fp;
	int ReadOnly;
	int Rerecords;
	int Size;
	int Frames;
	const char* filename;
};

struct MovieBufferStruct
{
	int size;
	char* data;
};

struct MovieBufferStruct ReadMovieIntoABuffer(FILE* fp);

void MovieLoadState(const char * filename);

void SaveMovieInState(FILE* fp, IOCheck_struct check);
void ReadMovieInState(FILE* fp); 

void TestWrite(struct MovieBufferStruct tempbuffer);

void MovieToggleReadOnly(void);

void TruncateMovie(struct MovieStruct Movie);

void DoFrameAdvance(void);

int SaveMovie(const char *filename);
int PlayMovie(const char *filename);
void StopMovie(void);

void MakeMovieStateName(const char *filename);

void MovieReadState(FILE* fp, const char * filename);

void PauseOrUnpause(void);

extern int framecounter;
extern int LagFrameFlag;
extern int lagframecounter;
extern char MovieStatus[40];
extern char InputDisplayString[40];
extern int FrameAdvanceVariable;
#endif
