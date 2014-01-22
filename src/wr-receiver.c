/*
 * Author: Miguel Jiménez López <klyone@correo.ugr.es>
 *
 * Supervisor: Javier Díaz Alonso <jda@ugr.es>
 *
 * University: University of Granada
 *
 * Version: 1.0
 *
 * Released to the public domain as sample code to be customized.
 *
 * It has been performed in framework of "Beca de la iniciación a la investigación" of University of Granada. 
 *
 * This program is inspired in wr-agent and wr-ruler Alessandro Rubbini's codes. <link>
 */


#include "wr-rec-send.h"

/* And a simple main with the loop inside */
int main(int argc, char **argv)
{
   frame f;
   int sock;
   char buffer[MAX_BUFFER];
   socket_connection connection;
   int rcode;
   wr_timer timer;
   int localhost = 1;
   int type_package;
   int size_package;
   long int npackages_received = 0;
   long int npackages_corrupted = 0;
   long int bits_transmited = 0;
   
   int lost_packages = 0;
   double lost_packages_ratio = 0;
   double bandwidth = 0;
   double bandwidth_ratio = 0;

   long int npackages_burst = 0;

   int end = 0;
   
   if (argc != 3) {
      fprintf(stderr, "%s: Use \"%s <wr-if> <localhost> \"\n",
         argv[0], argv[0]);
      exit(1);
   }
   
   int sockfd, newsockfd, portno;
   socklen_t clilen;
   struct sockaddr_in serv_addr, cli_addr;

   // --------------- Initialize server TCP --------------------------------- //
   
   sockfd = socket(AF_INET, SOCK_STREAM, 0);
   if (sockfd < 0) {
        perror("ERROR opening TCP socket");
   exit(1);
   }
   bzero((char *) &serv_addr, sizeof(serv_addr));
   portno = WR_PORT;
   serv_addr.sin_family = AF_INET;
   serv_addr.sin_addr.s_addr = INADDR_ANY;
   serv_addr.sin_port = htons(portno);

  // Avoid error of TIME WAIT state (in TCP)
  int opt = 1;
  setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));


   if (bind(sockfd, (struct sockaddr *) &serv_addr,
              sizeof(serv_addr)) < 0) {
              perror("ERROR on binding TCP socket");
         exit(1);
   }
   listen(sockfd,5);
   clilen = sizeof(cli_addr);
   
   // ----------------------------------------------------------------------- //

   // Convert localhost flag (string) into integer
   sscanf(argv[2],"%d",&localhost);

   // Main loop
   while (end == 0) {

	  // Open WR socket for RAW packets
      if ((rcode = open_wr_sock(argv[1],&connection,USE_WR_SPEC)) < 0) {
	    switch (rcode) {

		case -1:
		   perror("ERROR on opening RAW socket");
		break;

		case -2:
		   perror("ERROR device is not a SPEC card");
		break;

		case -3:
		   perror("ERROR to get MAC address");
		break;

		case -4:
		   perror("ERROR to get interface index");
		break;

		case -5:
		   perror("ERROR to bind RAW socket");
		break;

	    };
            exit(1);
      }
      sock = connection.socket;
      
     // -------------------- Open TCP connection --------------------
     
     newsockfd = accept(sockfd, 
                 (struct sockaddr *) &cli_addr, 
                 &clilen);
     if (newsockfd < 0) 
          perror("ERROR on accept TCP socket");
          
   // ----------------- TCP connection ---------------------------

    // If it's not localhost, send mac addr
    if (localhost == 0) {
      send(newsockfd,connection.macaddr_local,ETH_ALEN,0);
    }
      
    // Receive size_f and npackages from sender
    recv(newsockfd,&size_package,sizeof(int),0);
    recv(newsockfd,&npackages_burst,sizeof(long int),0);
    
    // Send ACK to sender
    send(newsockfd,"ACK",4,0);
   
      // Run timer
      startTimer(&timer);

      while (1) {

        // Receive frame
        if ((rcode = receive_frame(sock,&f)) == 0) {
			// Check packet content
            if (check_csum(&f) != 1) {
			   // if it's wrong, mark it as corrupted
               npackages_received++;
               npackages_corrupted++;
            }
            else {
				type_package = ntohs(f.wr_h.type_f);
               
				// if Data arrives, it counts new packet
				if (type_package == WR_PACKET_DATA) {
					npackages_received++;
				}
				else {
					if((type_package == WR_PACKET_END_BURST || type_package == WR_PACKET_TERMINATE)) {
						// Stop timer
                        stopTimer(&timer);
                        
                        send(newsockfd,"ACK",4,0);

                        // STADISTICS
            
                        measure_performance(size_package,npackages_burst,npackages_received,timer,
                                          &lost_packages,&lost_packages_ratio,&bandwidth,&bandwidth_ratio,0);


                        // Send bandwidth, lost and corrupted packages

                        send(newsockfd,&bandwidth,sizeof(double),0);
                        send(newsockfd,&lost_packages,sizeof(long int),0);
						send(newsockfd,&npackages_corrupted,sizeof(long int),0);

                        // Wait ACK from sender

                        recv(newsockfd,buffer,4,0);

                        // reset packets received

                        npackages_received = 0;

						// Send ACK to sender

						send(newsockfd,"ACK",4,0);


						if (type_package == WR_PACKET_TERMINATE)
							break;

                       // Run timer

                       startTimer(&timer);
				}
      
               }
            
            }

            
   }
       
}

   // Receives last packet (it indicates if receiver will finish or not)
   recv(newsockfd,buffer,10,0);
   
   // if packet is TERMINATE, receiver finishes
   if (strcmp(buffer,"TERMINATE") == 0) {
      end = 1;
   }
   // if packet is WAIT, receiver resets
   else {
      if (strcmp(buffer,"WAIT") == 0) {
         end = 0;
      }
   }

   // Close sockets (new connection / end)
   close(newsockfd);
   close(sock);
 }
 
   // Close TCP socket
   close(sockfd);
   
   exit(0);
}
