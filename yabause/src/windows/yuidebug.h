#ifndef YUIDEBUG_H
#define YUIDEBUG_H
#include "../sh2core.h"

enum { OFN_DEFAULTSAVE=0, OFN_DEFAULTLOAD };

LRESULT CALLBACK MemTransferDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
                                    LPARAM lParam);
LRESULT CALLBACK SH2DebugDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
                                 LPARAM lParam);
LRESULT CALLBACK VDP1DebugDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
                                  LPARAM lParam);
LRESULT CALLBACK VDP2DebugDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
                                  LPARAM lParam);
LRESULT CALLBACK M68KDebugDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
                                  LPARAM lParam);
LRESULT CALLBACK SCUDSPDebugDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
                                    LPARAM lParam);
LRESULT CALLBACK SCSPDebugDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
                                  LPARAM lParam);
LRESULT CALLBACK MemoryEditorDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
                                     LPARAM lParam);
LRESULT CALLBACK ErrorDebugDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
                                    LPARAM lParam);
LRESULT CALLBACK AboutDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
                              LPARAM lParam);
LRESULT CALLBACK LogDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
                            LPARAM lParam);
void UpdateLogCallback (char *string);
void SetupOFN(OPENFILENAME *ofn, int type, HWND hwnd, const char *lpstrFilter, char *lpstrFile, DWORD nMaxFile);

extern SH2_struct *debugsh;
extern HWND LogWin;
extern char *logbuffer;
extern u32 logsize;
#endif
