#include "stub.h"
#include "client.h"
#include "../sh2core.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>

typedef struct
{
   SH2_struct * context;
   int server;
} gdb_stub;

static void * gdb_stub_client(void * data)
{
   gdb_client * client = data;
   char buffer[1024];
   ssize_t got;
   gdb_packet packet;

   packet.state = 0;
   packet.client = client;

   while(-1 != (got = recv(client->sock, buffer, 1024, 0)))
   {
      gdb_packet_read(&packet, buffer, got);
   }
}

static void * gdb_stub_listener(void * data)
{
   gdb_stub * stub = data;
   int sock;

   while(1)
   {
      pthread_t t;
      gdb_client * client;

      sock = accept(stub->server, NULL, NULL);
      if (sock == -1)
      {
         perror("accept");
         return NULL;
      }

      client = malloc(sizeof(gdb_client));
      client->context = stub->context;
      client->sock = sock;
      pthread_cond_init(&client->cond, NULL);

      SH2SetBreakpointCallBack(stub->context, gdb_client_lock, client);

      pthread_create(&t, NULL, gdb_stub_client, client);
   }

   return NULL;
}

int GdbStubInit(SH2_struct * context, int port)
{
   struct sockaddr_in addr;
   int opt = 1;
   int server;

   server = socket(AF_INET, SOCK_STREAM, 0);

   if (setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int)) == -1)
   {
       perror("setsockopt");
       return -1;
   }

   memset(&addr, 0, sizeof(struct sockaddr_in));
   addr.sin_family = AF_INET;
   addr.sin_addr.s_addr = INADDR_ANY;
   addr.sin_port = htons(port);
   if (bind(server, (struct sockaddr *) &addr, sizeof(addr)) == -1)
   {
       fprintf(stderr, "Can't bind to port %d: %s\n", port, strerror(errno));
       return -1;
   }

   if (listen(server, 3) == -1)
   {
       perror("listen");
       return -1;
   }

   {
      pthread_t t;
      gdb_stub * stub;
      stub = malloc(sizeof(gdb_stub));
      stub->context = context;
      stub->server = server;

      pthread_create(&t, NULL, gdb_stub_listener, stub);
   }

   return 0;
}

int GdbStubDeInit()
{
}
