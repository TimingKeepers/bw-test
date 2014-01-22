/*
 * Author: Miguel Jiménez <klyone@hotmail.com>
 *
 * Released to the public domain as sample code to be customized.
 *
 * This work is part of the White Rabbit project, a research effort led
 * by CERN, the European Institute for Nuclear Research.
 */

#include "wr-rec-send.h"

/* Finally, a main function to wrap it all */
int main(int argc, char **argv)
{
   int localhost = 1;
   socket_connection connection;
   int sock;
   int i;
   int j;
   
   double latency;
   double latency_average = 0;

   // Check if number of arguments is correct
   if (argc != 2) {
      fprintf(stderr, "%s: Use \"%s <wr-if>\n",
         argv[0], argv[0]);
      exit(1);
   }
   
   // Open comunication's socket (it will send RAW Ethernet packets)
   if (open_wr_sock(argv[1],&connection,USE_WR_SPEC) < 0) {
      exit(1);
   }

   sock = connection.socket;
   
   for(j = 1 ; j <= 100 ; j++) {
      latency_average = 0;
      for(i = 1 ; i <= 1000 ; i++) {
         latency =  measure_latency_sender(sock,&connection,localhost);
         latency_average += latency;
         //printf("Link Latency: %lf ns \n",latency);
         //sleep(5); // wait 5 second
      }
      
      latency_average /= 1000;
   
      printf("Average Link Latency: %lf ns \n",latency_average);
   }

   close(sock);
   exit(0);
}
