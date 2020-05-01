#include "eeprom.h"
#include "memory.h"

//AK93C45/L 1024 bits 64registers*16bits
//Address length: 6 bits
//Data length: 16 bits
//Start pattern: 2 bits
//opcode: 2 bits
//Total: 26 bits/command(Read/write)

static u8 cs = 0;
static u8 clk = 0;
static u8 di = 0;
static u8 d_o = 0;

static u8 reg[64*2*2];
static u32 input; 
static u8 di_index = 0;

static u8 EW = 0;
static u8 cmd = -1;
static u16 ext_data;

static char eeprom_filename[4096];

#define EEPROM_LOG
#define EEPROM_CMD_LOG

static void executeCmd() {
  int i,j;
  d_o = 1;
  EEPROM_LOG("Cmd = %d\n", cmd);
  if (di_index == 22) {
    //Assume start_bit will be ok...
    u8 new_cmd = (input>>22)&0x3;
    cmd = -1;
    EEPROM_LOG("Reset Cmd = %x\n", cmd);
    switch (new_cmd) {
      case 0:
        cmd = 0;
        EW = (EW == 0)?1:0;
        EEPROM_LOG("Cmd = %x\n", cmd);
        break;
      default:
        cmd = (new_cmd == 2 ? 2 : (EW == 0)?-1:new_cmd);
        break;
    }
    EEPROM_LOG("NEw Cmd = %x\n", cmd);
  }
  if ((di_index == 0) && (cmd == 1)) {
    u8 addr = ((input>>16)&0x3F);
    u16 data = (input & 0xFFFF);
    //WRITE
    reg[addr*2+1] = (data & 0xFF);
    reg[addr*2] = (data>>8)&0xFF;
    EEPROM_CMD_LOG("WRITE 0x%04x@%d 0x%04x@%d\n", reg[addr*2], addr*2, reg[addr*2+1], addr*2+1);
    for (i =0; i< 0x8; i++) {
      for (j =0; j< 16; j++) {
        EEPROM_CMD_LOG("0x%02x,",reg[i*16+j]);
      }
      EEPROM_CMD_LOG("\n");
    }
  }
  if ((di_index == 16) && (cmd == 2)) {
    u8 addr = ((input>>16)&0x3F);
    //READ
    ext_data = reg[addr*2+1] | (reg[addr*2]<<8);
    EEPROM_CMD_LOG("READ ext_data=0x%04x@%d\n", ext_data, addr*2);
  }
  if ((di_index < 16) && (cmd == 2)) {
    //READ
    d_o = (ext_data >> di_index) & 0x1;
    EEPROM_LOG("READ do = ext_data[%d] = %x\n", di_index, d_o);
  }
  if ((di_index == 16) && (cmd == 3)) {
    u8 addr = ((input>>16)&0x3F);
    //ERASE
    reg[addr*2] = 0;
    reg[addr*2+1] = 0;
    EEPROM_CMD_LOG("ERASE @%d @%d\n", addr*2, addr*2+1);
  }
}

void eeprom_set_clk(u8 state) {
  state &= 1;
  if (state == clk)
    return;
  if (cs == 1) {
    if (state == 1) {
      //Rising edge
      if (di_index >= 0) {
        u32 mask = ~(1<<di_index);
        input &= mask;
        input |= (di<<di_index);
        EEPROM_LOG("Exec data[%d] = %d, addr=%x\n", di_index, di, input);
        executeCmd();
        di_index -= 1;
      }
    }
  }
  clk = state;
}

void eeprom_set_di(u8 state) {
  di = state & 1;
}

void eeprom_set_cs(u8 state) {
  state &= 1;
  if (state == cs)
    return;
  EEPROM_LOG("Cs to %x\n", state & 1);
  if (state == 1) {
    //Rising edge
    input = 0;
    di_index = 25;
    EEPROM_LOG("Clear cmd %d %d\n", state, cs);
    cmd = -1;
  } else {
    d_o = 1;
  }
  cs = state;
}

int eeprom_do_read(void) {
  EEPROM_LOG("Read d_o = %x\n", d_o);
  return d_o;
}

void eeprom_init(const char* filename) {
  memcpy(eeprom_filename, filename, sizeof(eeprom_filename));
  T123Load((u8*)reg, 0x80, 1, eeprom_filename);
}

void eeprom_deinit() {
  T123Save((u8*)reg, 0x80, 1, eeprom_filename);
}

void eeprom_start(const u8* table) {
  int i,j;
  u8 init[128] = {
    0x53,0x45,0x47,0x41,0xff,0xff,0xff,0xff,0xdf,0xf9,0xff,0xff,0x00,0x00,0x00,0x04,
    0x01,0x00,0x01,0x01,0x00,0x00,0x00,0x00,0x00,0x08,0x08,0xfd,0x01,0x01,0x01,0x02,
    0x02,0x02,0x02,0x01,0x02,0x02,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
    0xff,0xff,0xff,0xff,0xdf,0xf9,0xff,0xff,0x00,0x00,0x00,0x04,0x01,0x00,0x01,0x01,
    0x00,0x00,0x00,0x00,0x00,0x08,0x08,0xfd,0x01,0x01,0x01,0x02,0x02,0x02,0x02,0x01,
    0x02,0x02,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};

  if (table == NULL) {
    memcpy((u8*)reg, init, 0x80);
  } else {
    memcpy((u8*)reg, table, 0x80);
  }
  for (i =0; i< 0x8; i++) {
    for (j =0; j< 16; j++) {
      EEPROM_CMD_LOG("0x%02x,",reg[i*16+j]);
    }
    EEPROM_CMD_LOG("\n");
  }
}
