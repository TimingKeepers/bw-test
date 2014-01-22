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

/* Finally, a main function to wrap it all */
int main(int argc, char **argv)
{
    
   frame f;
   int iargv = 2;
   int localhost = 1;
   char buffer[MAX_BUFFER];
   int rcode = 0;
   int rv;
   long int nmessg = 0;
   int actualburst = 0;
   int kill = 0;
   int nburst = BURSTN;
   int usec = 1;
   socket_connection connection;
   int sock;
   int type_package;
   double bandwidth;
   long int lost_packages;

   FILE * log_file = init_log_file(LOG_FILE_DEFAULT);

   unsigned short max_s = MAX_FRAME_SIZE-sizeof(wr_header);
   // Size of packet (default)
   unsigned short size_f = max_s;

   // Number of packages to send 
   long int npackages = BURST_NPACKAGES;
   long int npackages_corrupted;

   // Check if number of arguments is correct
   if (argc != 7 && argc != 8) {
      fprintf(stderr, "%s: Use \"%s <wr-if> <period (us)> <size_package> <number_packages> <number_burst> <kill_receiver>\n",
         argv[0], argv[0]);
      fprintf(stderr, "%s: Use \"%s <wr-if> <receiverhostname> <period (us)> <size_package> <number_packages> <number_burst> <kill_receiver>\n",
         argv[0], argv[0]);
      exit(1);
   }
   
   // Open comunication's socket (it will send RAW Ethernet packets)
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
   
   if(argc == 8) {
      localhost = 0;
      iargv+=1;
   }

 
    // -------------------- Open TCP connection --------------------
   
    int sockfd, portno;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    int flags;
   
    portno = WR_PORT;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)  {
        perror("ERROR opening TCP socket");
   exit(1);
    }
    if(localhost == 1)
       server = gethostbyname("localhost"); // change
    else
   	server = gethostbyname(argv[3]);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(1);
    }

    
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
        perror("ERROR connecting TCP socket");
   exit(1);
    }
        
   // ----------------- TCP connection ---------------------------

   // Convert time (string) into integer
   sscanf(argv[iargv],"%d",&usec);
   
   iargv++;

   // Convert size of package (string) into integer
   sscanf(argv[iargv],"%d",&size_f);

   if (size_f > max_s) {
	printf("Error: Max size of frame exceeded (<= 1490) \n");
	exit(1);
   }

   iargv++;
   
   // Convert number of packages (string) into integer
   sscanf(argv[iargv],"%d",&npackages);

   iargv++;
   
   // Convert number of packages (string) into integer
   sscanf(argv[iargv],"%d",&nburst);

   iargv++;
   
   // Convert kill receiver's flag (string) into integer
   sscanf(argv[iargv],"%d",&kill);

   // Write headers in log file
   write_header_log_file(log_file,size_f,npackages,usec,nburst);

   // Close log file
   close_log_file(log_file);
  
   // Receive MAC from receiver if it's not localhost
   if (localhost == 0)
	recv(sockfd,connection.macaddr_dest,ETH_ALEN,0);

   // Send size of frame
   send(sockfd,&size_f,sizeof(int),0);
   

   // Send number of packages in burst to receiver
   send(sockfd,&npackages,sizeof(long int),0);
    

   // Wait for ACK
   recv(sockfd,buffer,4,0);
   
   if (strcmp(buffer,"ACK") != 0) {
      printf("ACK not found \n");
      exit(1);
   }

   // Start to send burst of packages
   while(1) {
	  // Generate random data array
      generate_random_msg(buffer,size_f);
	  // Build WR header for packet
      build_data_frame(&f,&connection,buffer,localhost);
	  // Sends packet
      if(send_frame(sock,&f) == -1)
         fprintf(stderr, "%s: send(): %s\n",
         argv[0], strerror(errno));
		 
	  // Count packet
      nmessg++;

     
	  // if emission period is not zero, sender waits
      if(usec != 0)
         usleep(usec);   

	  // if sender sends last packet of burst...
      if(nmessg == npackages) {
		 // Reset count of packets and count a new burst
         nmessg = 0;
         actualburst++;
		 // if test ends...
         if (nburst != 0 && actualburst == nburst) {
		    // Sender build TERMINATE packet and sends it
			build_terminate_frame(&f,&connection,localhost);
			if(send_frame(sock,&f) == -1)
				fprintf(stderr, "%s: send(): %s\n",
					argv[0], strerror(errno));
       
		   // Sender waits for receiver ACK and if Timeout expires, sender re-sends TERMINATE packet
           wait_for_ack(sockfd,sock,&f);
            
            // Receive bandwidth and lost packages
            rv = recv(sockfd,&bandwidth,sizeof(double),0);
            if (rv < 0)
               perror("ERROR to receive BW information \n");
            rv = recv(sockfd,&lost_packages,sizeof(long int),0);
            if (rv < 0)
               perror("ERROR to receive lost packages information \n");
	    rv = recv(sockfd,&npackages_corrupted,sizeof(long int),0);
            if (rv < 0)
               perror("ERROR to receive corrupted packages information \n");

            // Write burst results into log file (Problem: Receiver's timer is running...)
            log_file = open_log_file(LOG_FILE_DEFAULT);   
            write_burst_results(log_file,lost_packages,bandwidth,npackages_corrupted);
            close_log_file(log_file);
            
            // ACK to receiver (finish to update log file)
            send(sockfd,"ACK",4,0);
            
            //Wait to ACK from receiver
            recv(sockfd,buffer,4,0);
      
            break; 
         }
    // Sender build ENDBURST packet and sends it     
   build_endburst_frame(&f,&connection,localhost);
         if(send_frame(sock,&f) == -1)
            fprintf(stderr, "%s: send(): %s\n",
               argv[0], strerror(errno));
            
		   // Sender waits for receiver ACK and if Timeout expires, sender re-sends ENDBURST packet
           wait_for_ack(sockfd,sock,&f);
            
         // Receive bandwidth and lost packages

	    rv = recv(sockfd,&bandwidth,sizeof(double),0);
            if (rv < 0)
               perror("ERROR to receive BW information \n");
            rv = recv(sockfd,&lost_packages,sizeof(long int),0);
            if (rv < 0)
               perror("ERROR to receive lost packages information\n");
	    rv = recv(sockfd,&npackages_corrupted,sizeof(long int),0);
            if (rv < 0)
               perror("ERROR to receive corrupted packages information \n");

            // Write burst results into log file (Problem: Receiver's timer is running...)
            log_file = open_log_file(LOG_FILE_DEFAULT);   
            write_burst_results(log_file,lost_packages,bandwidth,npackages_corrupted);
            close_log_file(log_file);
            
            // ACK to receiver (finish to update log file)
            send(sockfd,"ACK",4,0);
            
            //Wait to ACK from receiver
            recv(sockfd,buffer,4,0);
      }
   }

   // Sender sends TERMINATE/WAIT packet to receiver (TERMINATE: receiver will finish, WAIT: receiver will continue listening)
   if (kill == 1) {
       send(sockfd,"TERMINATE",10,0);
   }
   else {
       send(sockfd,"WAIT",5,0);
   }


   // Close all sockets
   close(sockfd);
   close(sock);
   exit(0);
}
