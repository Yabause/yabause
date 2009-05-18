#ifndef RAMWATCH_H
#define RAMWATCH_H   
#include "windows.h"
int ResetWatches();
void OpenRWRecentFile(int memwRFileNumber);
extern "C" {
extern bool AutoRWLoad;
extern bool RWSaveWindowPos;
}
#define MAX_RECENT_WATCHES 5
extern char rw_recent_files[MAX_RECENT_WATCHES][1024];
extern bool AskSave();
extern int ramw_x;
extern int ramw_y;
extern bool RWfileChanged;

// AddressWatcher is self-contained now
struct AddressWatcher
{
	unsigned int Address; // hardware address
	char Size;
	char Type;
	char* comment; // NULL means no comment, non-NULL means allocated comment
	int WrongEndian;
	unsigned int CurValue;
};
#define MAX_WATCH_COUNT 256
extern struct AddressWatcher rswatches[MAX_WATCH_COUNT];
extern int WatchCount; // number of valid items in rswatches

extern char Watch_Dir[1024];

int InsertWatch(const struct AddressWatcher *Watch, char *Comment);
int InsertWatchHwnd(const struct AddressWatcher *Watch, HWND parent); // asks user for comment //=NULL
extern "C" void Update_RAM_Watch();
int Load_Watches(int clear, const char* filename);

LRESULT CALLBACK RamWatchProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
extern HWND RamWatchHWnd;

#endif
