/*
 * Author: Miguel Jiménez <klyone@hotmail.com>
 *
 * Released to the public domain as sample code to be customized.
 *
 * This work is part of the White Rabbit project, a research effort led
 * by CERN, the European Institute for Nuclear Research.
 */

#include "wr-rec-send.h"

/* And a simple main with the loop inside */
int main(int argc, char **argv)
{
   int sock;
   socket_connection connection;
   int localhost = 1;
   int i = 0;
   int j = 0;
   
   if (argc != 2) {
      fprintf(stderr, "%s: Use \"%s <wr-if> \"\n",
         argv[0], argv[0]);
      exit(1);
   }
   

  /* All functions print error messages by themselves, so just exit */
  if (open_wr_sock(argv[1],&connection,USE_WR_SPEC) < 0)
      exit(1);

  sock = connection.socket;

  for(j = 1 ; j <= 100 ; j++) {
      for(i = 1 ; i <= 1000 ; i++) {
         measure_latency_receiver(sock,&connection,localhost);
         //sleep(5);
      }
  }

  close(sock);
   
  exit(0);
}
