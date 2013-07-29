#ifndef GDB_CLIENT_H
#define GDB_CLIENT_H

#include "packet.h"
#include "../sh2core.h"

struct _gdb_client
{
   SH2_struct * context;
   int sock;
   pthread_cond_t cond;
};

void gdb_client_received(gdb_client * client, gdb_packet * packet);
void gdb_client_error(gdb_client * client);
void gdb_client_send(gdb_client * client, const char * message, size_t msglen);
void gdb_client_break(gdb_client * client);
void gdb_client_lock(void *context, u32 addr, void * userdata);

#endif
