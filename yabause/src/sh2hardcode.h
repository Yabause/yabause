#ifndef _SH2HARDCODE_H_
#define _SH2HARDCODE_H_

#define N_HARD_BLOCS 7

typedef struct { 
  u32 addr;
  int hackSize;
  u16 *hack;
  void (*handler)(SH2_struct *context);
} hcTag;

extern hcTag hcTags[N_HARD_BLOCS];

extern int hackDelay;

void doCheckHcHacks();

static INLINE void checkHcHacks() {

  if ( ! hackDelay-- ) {
    doCheckHcHacks();
    hackDelay = 400;
  }
}

static INLINE void hcExec( SH2_struct *sh, u16 hcCode ) {

  hcTags[hcCode].handler(sh);
}

#endif
