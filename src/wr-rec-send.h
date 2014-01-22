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

#ifndef WR_RSEND_H
#define WR_RSEND_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <sys/types.h> 
#include <sys/poll.h>
#include <fcntl.h>
#include <netdb.h> 

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/if_ether.h>
#include <net/if.h>
#include <netpacket/packet.h>

/* The last two available are used for mezzanine-private stuff */
#define PRIV_MEZZANINE_ID   (SIOCDEVPRIVATE + 14)
#define PRIV_MEZZANINE_CMD   (SIOCDEVPRIVATE + 15) // -- NOT USED

#define WR_PROTO 0x5752 /* WR */
#define WR_PORT 2222
#define MAX_FRAME_SIZE 1514 // Ethernet MTU (omited CRC 4 bytes)
#define BURST_NPACKAGES 1000
#define BURSTN 10

// It is used to define if SPEC card (1, default) or Normal NIC (0) is used
#define USE_WR_SPEC 1

// Default name of log file
#define LOG_FILE_DEFAULT "log.tst"

// Maximum bandwidth's link (Gigabit Ethernet, 1 Gbps)
#define MAXIMUM_BANDWIDTH 1000000000

// Max size of buffer
#define MAX_BUFFER 3000

// 10 second of timeout
#define TIMEOUT 10000

// Type of package

typedef enum wr_type_pkg {
   WR_PACKET_DATA,
   WR_PACKET_LATENCY,
   WR_PACKET_END_BURST,
   WR_PACKET_TERMINATE,
} wr_type_pkg;

/*
*
* Define header of frame (includes size of data's frame)
*
* 1. eth_h: Ethernet's Header 
* 2. size_f: Size of data in packet
* 3. type_f: Type of WR's package
* 4. csum_header: header packet checksum
* 5. csum_msg: message packet checksum
*
*/

typedef struct wr_header {
   struct ether_header eth_h;
   short unsigned size_f;
   wr_type_pkg type_f;
   short unsigned csum_header;
   short unsigned csum_msg;
} wr_header;

/*
*
* Define frame that will travel by network. We only use a header (ethernet) and the message.
*
* 1. wr_h: WR's Header 
* 2. message: Body of Ethernet's packet
*
*/

typedef struct frame {
   wr_header wr_h;
   char messg[MAX_FRAME_SIZE];
} frame;

/*
*
* Define a struct that stores necessary data to send packages
*
* 1. socket: descriptor
* 2. macaddr_local: MAC direction source
* 2. macaddr_dest: MAC direction destination
*
*/

typedef struct socket_connection {
   int socket;
   unsigned char macaddr_local[ETH_ALEN];
   unsigned char macaddr_dest[ETH_ALEN];
} socket_connection;

/*
*
* Define a struct that stores necessary data to a timer
*

* 1. start_time: starting time
* 2. stop_time: termination time
* 3. duration: time elapses between starting time and termination time
*
* Note: *_d is same time but in double.
*
*/

typedef struct wr_timer {
   struct timespec start_time;
   struct timespec stop_time;
   struct timespec duration;
   double start_time_d;
   double stop_time_d;
   double duration_d;
} wr_timer;

/**
*
* init_log_file: Open log file to write (first time)
*
* Params:
*
* file_name: Log file path
*
* Code of errors:
*
* None
* 
*/

FILE * init_log_file(char * file_name) {
   
   FILE * fp;
   fp=fopen(file_name, "w+");

   return fp;
}

/**
*
* open_log_file: Open log file to write (to append info)
*
* Params:
*
* file_name: Log file path
*
* Code of errors:
*
* None
* 
*/

FILE * open_log_file(char * file_name) {
   
   FILE * fp;
   fp=fopen(file_name, "a+");

   return fp;
}

/**
*
* close_log_file: Close log file to save info
*
* Params:
*
* log_file: pointer to file
*
* Code of errors:
*
* Returns zero if the file is closed successfully.
* 
*/

int close_log_file(FILE * log_file) {

   return fclose(log_file);

}

/**
*
* write_header_log_file: Write headers in log file
*
* Params:
*
* log_file: pointer to file
* size_f: Packet's size (in bytes)
* npackages: Number of packages in each burst
* period: Sent period (packages)
* nburst: Number of burst
*
* Code of errors:
*
* Returns zero if it success
* 
*/

int write_header_log_file(FILE * log_file, unsigned short size_f, long int npackages, int period, int nburst) {
   char buffer[MAX_BUFFER];

   sprintf(buffer,"SIZE %d \n",size_f);
   fprintf(log_file, buffer);

   sprintf(buffer,"PERIOD %d \n",period);
   fprintf(log_file, buffer);

   sprintf(buffer,"NBURST %d \n",nburst);
   fprintf(log_file, buffer);

   sprintf(buffer,"NPACKG %d \n",npackages);
   fprintf(log_file, buffer);

   sprintf(buffer,"VALUES (BW,LOSTPACKG,CORRUPTEDPKG) \n");
   fprintf(log_file, buffer);

   return 0;
}

/**
*
* write_burst_results: Write burst results into log file
*
* Params:
*
* log_file: pointer to file
* lost_packages: Lost packages in burst
* bandwidth: Transfer ratio in burst
* npackages_corrupted: Corrupted packages in burst
*
* Code of errors:
*
* Returns zero if it success
* 
*/

int write_burst_results(FILE * log_file, int lost_packages, double bandwidth, long int npackages_corrupted) {
   char buffer[MAX_BUFFER];

   sprintf(buffer,"BURST %lf %lu %lu \n",bandwidth,lost_packages,npackages_corrupted);
   fprintf(log_file, buffer);

   return 0;
}


/**
*
* startTimer: Start to run a timer.
*
* Params:
*
* t: timer
*
* Code of errors:
*
* None
* 
*/


int startTimer(wr_timer * t) {
   clock_gettime(CLOCK_REALTIME,&(t->start_time));
   t->start_time_d = ((t->start_time).tv_sec * 1000000000.0) + ((t->start_time).tv_nsec); 

   return 0;
}

/**
*
* stopTimer: Stop a timer.
*
* Params:
*
* t: timer
*
* Code of errors:
*
* None
* 
*/


int stopTimer(wr_timer * t) {
   clock_gettime(CLOCK_REALTIME,&(t->stop_time));

   (t->duration).tv_sec = (t->stop_time).tv_sec - (t->start_time).tv_sec;
   (t->duration).tv_nsec = (t->stop_time).tv_nsec - (t->start_time).tv_nsec;
   t->duration_d = ((t->duration).tv_sec * 1000000000.0) + ((t->duration).tv_nsec); 
   t->stop_time_d = ((t->stop_time).tv_sec * 1000000000.0) + ((t->stop_time).tv_nsec); 

   return 0;
}

/**
*
* resetTimer: Reset a timer (same that startTimer).
*
* Params:
*
* t: timer
*
* Code of errors:
*
* None
* 
*/


int resetTimer(wr_timer * t) {
   startTimer(t);
   return 0;
}

/**
*
* isTimeout: Check if timer has raised timeout (Need timer is running, Needn't a previous call to stopTimer)
*
* Params:
*
* t: timer
* ms: time in ms
*
* Code of errors:
*
* None
*
* Returns 1 if timeout has raised or 0 if not.
* 
*/

int isTimeout(wr_timer *t, long int ms) {
   double tactual;
   double tactualms;
   stopTimer(t);
   
   tactual = t->duration_d;
   tactualms = tactual/1000000; //ms
   
   if(tactualms > ms) {
      return 1;
   }
   else
      return 0;
}

/**
*
* get_info_SPEC: it requests SPEC card information
*
* Params:
*
* sock: socket's descriptor
* ifr: struct contains request's data
*
* Code of errors:
*
* -1: Error: There is not SPEC card connected
*
* NOTE: you must put interface's name in ifreq struct. You may use strcpy as
* following: 'strncpy(ifr.ifr_name, name, sizeof(ifr.ifr_name))'. Moreover,
* it's better if you put ifreq struct to zero before you load any data.
*
* NOTE2: This function only can use to check if there is a SPEC card in the
* interface. In the future, it will return all information about SPEC card.
*
*/

int get_info_SPEC(int sock, struct ifreq * ifr) {
   if (ioctl(sock, PRIV_MEZZANINE_ID, ifr) < 0
       /* EAGAIN is special: it means we have no ID to check yet */
      && errno != EAGAIN) {
      close(sock);
      return -1;
   }
   
   return 0;
}

/**
*
* get_mac: it requests mac address
*
* Params:
*
* sock: socket's descriptor
* ifr: struct contains request's data
*
* Code of errors:
*
* -1: Error to get mac address
*
* NOTE: you must put interface's name in ifreq struct. You may use strcpy as
* following: 'strncpy(ifr.ifr_name, name, sizeof(ifr.ifr_name))'. Moreover,
* it's better if you put ifreq struct to zero before you load any data.
*
*/

int get_mac(int sock,struct ifreq * ifr) {
   /* Retrieve the local MAC address to send correct Ethernet frames */
   if (ioctl(sock, SIOCGIFHWADDR, ifr) < 0) {
      close(sock);
      return -1;
   }
   return 0;
}

/**
*
* get_index: it requests interface's index
*
* Params:
*
* sock: socket's descriptor
* ifr: struct contains request's data
*
* Code of errors:
*
* -1: Error to get interface's index
*
* NOTE: you must put interface's name in ifreq struct. You may use strcpy as
* following: 'strncpy(ifr.ifr_name, name, sizeof(ifr.ifr_name))'. Moreover,
* it's better if you put ifreq struct to zero before you load any data.
*
*/

int get_index(int sock,struct ifreq * ifr) {
   /* Retieve the interfaceindex */
   if (ioctl(sock, SIOCGIFINDEX, ifr) < 0) {
      close(sock);
      return -1;
   }
   return 0;
}

/**
*
* open_wr_sock: It opens a socket to be able to send/receive RAW Ethernet Packages
*
* Params:
*
* name: interface's name
* connection: struct will store connection's information
* spec: check if it's SPEC card (and retrieve its information)
*
* Code of errors:
*
* -1: Error to open socket
* -2: device of interface is not SPEC card
* -3: Error to get MAC address
* -4: Error to get Index of interface
* -5: Error to bind socket
*
*/
int open_wr_sock(char *name, socket_connection * connection, int spec)
{
   struct ifreq ifr;
   struct sockaddr_ll addr;
   int sock, ifindex;
   unsigned char macaddr[ETH_ALEN];

   sock = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
   if (sock < 0) {
      return -1;
   }
   
   memset(&ifr, 0, sizeof(ifr));
   strncpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));

   if(spec == 1) {
      if(get_info_SPEC(sock,&ifr) < 0)
             return -2;
   }

   if(get_mac(sock,&ifr) < 0)
      return -3;
  
   memcpy(macaddr, ifr.ifr_hwaddr.sa_data,
          sizeof(macaddr));

   if(get_index(sock,&ifr) < 0)
      return -4;
  
   ifindex = ifr.ifr_ifindex;

   /* Bind to the interface, so to be able to send */
   memset(&addr, 0, sizeof(addr));
   addr.sll_family = AF_PACKET;
   addr.sll_protocol = htons(WR_PROTO);
   addr.sll_ifindex = ifindex;
   if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
      close(sock);
      return -5;
   }

   connection->socket = sock;
   memcpy(connection->macaddr_local,macaddr,sizeof(macaddr));

   return 0;
}

/**
*
* measure_performance: Calculate bandwidth and lost packages in burst
*
* Params:
*
* size_package: Number of bytes of package
* npackages_burst: Number of total packages sent in burst
* npackages_receiver: Number of packages received in burst
* timer: Indicate elapsed time
* lost_packages: Number of lost packages in burst (out)
* lost_packages_ratio: Number of lost packages in burst over total packages (out)
* bandwidth: Bandwidth raised in burst (out)
* bandwidth_ratio: Bandwidth raised in burst over total bandwidth (out)
* print: show stadistics in screen (==1) or not (otherwise)
*
* Code of errors:
*
* None
*
*/

void measure_performance(int size_package,long int npackages_burst,long int npackages_received,wr_timer timer,int * lost_packages, double * lost_packages_ratio,double * bandwidth,double * bandwidth_ratio, int print) {

   long int bits_transmited = 0;

   // STADISTICS
            
    // lost packages
           
    (*lost_packages) = npackages_burst-npackages_received;
    (*lost_packages_ratio) = ((*lost_packages)*1.0)/npackages_burst;

            
    // Check if size of package is greater than 40
            
    if (size_package < 40)
      size_package = 40;
               
    bits_transmited = (size_package+sizeof(struct wr_header))*8*npackages_received; 

    // bandwidth
            
    (*bandwidth) = (bits_transmited*1.0)/(timer.duration_d/1000); // in Mbps
    (*bandwidth_ratio) = (*bandwidth)/1000; // % over 1 Gbps

  // Print STADISTICS
    
    if (print == 1) {
       printf("\n ------------------------------------------------------------------------ \n\n");
       printf("\t Packages in burst: %d \n",npackages_burst);
       printf("\t Lost packages: %d (ratio: %f percent) \n",(*lost_packages),(*lost_packages_ratio)*100);
       printf("\t Bandwidth: %f Mbps (%f percent) \n",(*bandwidth),(*bandwidth_ratio)*100);
       printf("\n ------------------------------------------------------------------------ \n");
    }

}

/**
*
* build_header: Build Ethernet header for packages
*
* Params:
*
* f: package where header will be
* connection: struct will store connection's information
* size_f: Size of data's frame
* type_f: Type of package
* localhost: indicate if packet is receiver and send are located in same host (1) or not (0)
*
* Code of errors:
*
* None
*
*/

void build_header(frame * f,socket_connection * connection,int size_f,wr_type_pkg type_f,int localhost) {
   f->wr_h.size_f = htons(size_f);
   f->wr_h.type_f = htons(type_f);
   
   if (localhost == 0) 
      memcpy(&f->wr_h.eth_h.ether_dhost,connection->macaddr_dest, ETH_ALEN);
   else
      memset(&f->wr_h.eth_h.ether_dhost,0xff, ETH_ALEN);
      
   memcpy(&f->wr_h.eth_h.ether_shost, connection->macaddr_local, ETH_ALEN);
   f->wr_h.eth_h.ether_type = ntohs(WR_PROTO);
   return;
}

/**
*
* csum: Build Checksum code from data array
*
* Params:
*
* buf: data array
* nwords: Number of 16b words of data array
*
* Code of errors:
*
* None
*
* Return: Checksum code
*
*/

unsigned short csum(unsigned short *buf,unsigned long nwords) {
   //this function returns the checksum of a buffer
   unsigned long sum = 0;
   unsigned long i;
   
   for(i = 0 ; i < nwords ; i++) {
      sum += buf[i];
   }
   
   sum = (sum >> 16) + (sum & 0xffff);
   sum += (sum >> 16);
   return (unsigned short) (~sum);
}

/**
*
* build_checksum: Build Checksum code from frame (packet) and initialize csum_header and csum_msg fields
*
* Params:
*
* f: data packet
*
* Code of errors:
*
* None
*
*/

void build_checksum(frame * f) {
   unsigned long nwords_header = ((sizeof(wr_header))/sizeof(unsigned short))-2;
   unsigned long nwords_msg = (ntohs(f->wr_h.size_f)/sizeof(unsigned short));
   
   f->wr_h.csum_header = htons(csum((unsigned short *) f,nwords_header));
   f->wr_h.csum_msg = htons(csum((unsigned short *) (f->messg),nwords_msg));
   
}

/**
*
* check_csum: Build Check if packet has been received corretly
*
* Params:
*
* f: data packet
*
* Code of errors:
*
* None
*
* Return: 1 if packet is correct and 0 otherwise
*
*/

int check_csum(frame * f) {
    frame f2 = *f;
    
    build_checksum(&f2);
    
    unsigned short cs2_header = ntohs(f2.wr_h.csum_header);
    unsigned short cs1_header = ntohs(f->wr_h.csum_header);
    
    unsigned short cs2_msg = ntohs(f2.wr_h.csum_msg);
    unsigned short cs1_msg = ntohs(f->wr_h.csum_msg);
    
    if ((cs2_msg == cs1_msg) && (cs2_header == cs1_header)) {
      return 1;
    }
    else {
      return 0;
    }
}

/**
*
* build_terminate_frame: Build TERMINATE Ethernet package
*
* Params:
*
* f: package to build
* connection: struct will store connection's information
* localhost: indicate if packet is receiver and send are located in same host (1) or not (0)
*
* Code of errors:
*
* None
*
*/

void build_terminate_frame(frame * f, socket_connection * connection, int localhost) {
    build_header(f,connection,9,WR_PACKET_TERMINATE,localhost);
    sprintf(f->messg,"TERMINATE");
    build_checksum(f);
}

/**
*
* build_endburst_frame: Build ENDBURST Ethernet package
*
* Params:
*
* f: package to build
* connection: struct will store connection's information
* localhost: indicate if packet is receiver and send are located in same host (1) or not (0)
*
* Code of errors:
*
* None
*
*/

void build_endburst_frame(frame * f, socket_connection * connection, int localhost) {
    build_header(f,connection,8,WR_PACKET_END_BURST,localhost);
    sprintf(f->messg,"ENDBURST");
    build_checksum(f); 
}

/**
*
* build_data_frame: Data Ethernet package
*
* Params:
*
* f: package to build
* connection: struct will store connection's information
* data: Information to send
* localhost: indicate if packet is receiver and send are located in same host (1) or not (0)
*
* Code of errors:
*
* None
*
*/

void build_data_frame(frame * f, socket_connection * connection, char * data, int localhost) {
   int i;
   build_header(f,connection,strlen(data),WR_PACKET_DATA,localhost);
    for (i = 0 ; i < ntohs(f->wr_h.size_f) ; i++) {
       f->messg[i] = data[i];
    }
    build_checksum(f); 
}

/**
*
* build_latency_frame: Latency Data Ethernet package
*
* Params:
*
* f: package to build
* connection: struct will store connection's information
* tlatency: counter to measure latency
* localhost: indicate if packet is receiver and send are located in same host (1) or not (0)
* start_stop: indicate if it have to start/stop timer
*
* Code of errors:
*
* None
*
*/

void build_latency_frame(frame * f, socket_connection * connection, wr_timer * tlatency, int localhost,int start_stop) {
   char buffer[50];
   if (start_stop == 0) {
      startTimer(tlatency);
      sprintf(buffer,"%lf",tlatency->start_time_d);
   }
   else {
      stopTimer(tlatency);
      sprintf(buffer,"%lf",tlatency->duration_d);
   }
   build_header(f,connection,strlen(buffer),WR_PACKET_LATENCY,localhost);
   sprintf(f->messg,"%s",buffer);
   build_checksum(f); 
}

/**
*
* send_frame: it sends a RAW Ethernet Package
*
* Params:
*
* sender_sock: socket's descriptor
* frame: package will be sent
*
* Code of errors:
*
* -1: Error to send package
*
*/

int send_frame(int sender_sock, frame * f) {
   int size_f = ntohs((f->wr_h).size_f);
   int size_header = sizeof(f->wr_h);
   int size_tot = size_header + size_f;
   int bytes_sent = 0;

   bytes_sent = write(sender_sock,(void *)f,size_tot);
   if (bytes_sent < 0) {
         printf("Error: package send() fails \n");
         return -1;
   }

   return 0;
}

/**
*
* receive_frame: it receives a RAW Ethernet Package
*
* Params:
*
* receiver_sock: socket's descriptor
* frame: package will be received
*
* Code of errors:
*
* -1: Error to receive package
* -2: Error to receive package (its protocol is not WR PROTOCOL)
*
*/

int receive_frame(int receiver_sock, frame * f) {
   int bytes_received;
   int i;

  
   bytes_received = read(receiver_sock,(void *)f,MAX_FRAME_SIZE);


   unsigned short size_f = ntohs((f->wr_h).size_f);

   
   if (bytes_received < 0) {
      perror("Error: recv() fails \n");
      return -1;
   }

   if (ntohs(((f->wr_h).eth_h).ether_type) != WR_PROTO) {
      perror("Error: package is not WR protocol \n");
      return -2;
   }
   
   return 0;
}

/**
*
* generate_random_msg: it generates a data buffer with random elements
*
* Params:
*
* buffer: Data buffer
* size: Number of elements of data buffer
*
*
*/

void generate_random_msg(char * buffer, int size) {
   int i;

   for(i = 0 ; i < size ; i++) {
      buffer[i] = 48+rand()%10;
   }

  buffer[i] = 0; 

}

/**
*
* wait_for_ack: Process waits for ack and re-send a frame if timeout 
*               is reached.
*
* Params:
*
* sockack: socket to receive ack
* sockrenv: socket to re-send frame
* freenvio: frame to re-send
*
*/

int wait_for_ack(int sockack,int sockrenv, frame * freenvio) {
    int ok = 0;
    int rv;
    char buffer[256];
    struct pollfd polling[1];
    polling[0].fd = sockack;
    polling[0].events = POLLIN;
            
    while (!ok) {
      rv = poll(polling,1,TIMEOUT);
         if (rv > 0) {
            recv(sockack,buffer,4,0);
            if (strcmp(buffer,"ACK") == 0) {
               ok = 1;
            }
         }
         else {
            if(send_frame(sockrenv,freenvio) == -1)
               fprintf(stderr, "wait_for_ack: send(): %s\n",strerror(errno));
         }
    }
    
    return 0;
}

/**
*
* measure_latency_sender: Protocol to mesure latency (it must be executed by sender program)
*
* Params:
*
* sock: socket to send and receive WR packet
* connection: Connection information of WR socket
* localhost: Indicates if sender and receiver are in same PC (1) or not (0).
*
* Code of errors:
*
* None
*
* Return: Latency link
*
*/

double measure_latency_sender(int sock, socket_connection * connection, int localhost) {
   double latency_sender;
   double latency_receiver;
   double latency;
   wr_timer tlatency;
   frame f;
   
   // Build latency link frame
   build_latency_frame(&f,connection,&tlatency,localhost,0);
   
   // Send latency frame
   if(send_frame(sock,&f) == -1)
               fprintf(stderr, "measure_latency: send(): %s\n",strerror(errno));
               
   // Receive latency echo frame
   receive_frame(sock,&f);
   
   // Build latency frame and calculate latency (sender)
   build_latency_frame(&f,connection,&tlatency,localhost,1);
   
   // Receive latency frame with latency (receiver)
   if(send_frame(sock,&f) == -1)
               fprintf(stderr, "measure_latency: send(): %s\n",strerror(errno));
               
   receive_frame(sock,&f);

   sscanf(f.messg,"%lf",&latency_receiver);
   
   latency_sender = tlatency.duration_d;
   
   latency = (latency_receiver+latency_sender)/2.0;
   
   return latency;
}

/**
*
* measure_latency_sender: Protocol to mesure latency (it must be executed by receiver program)
*
* Params:
*
* sock: socket to send and receive WR packet
* connection: Connection information of WR socket
* localhost: Indicates if sender and receiver are in same PC (1) or not (0).
*
* Code of errors:
*
* None
*
* Return: Latency link
*
*/

void measure_latency_receiver(int sock, socket_connection * connection, int localhost) {
   wr_timer tlatency;
   frame f;
   
   // Receive latency
   receive_frame(sock,&f);
   
   // Build latency link frame
   build_latency_frame(&f,connection,&tlatency,localhost,0);
   
   // Send latency frame
   if(send_frame(sock,&f) == -1)
               fprintf(stderr, "measure_latency: send(): %s\n",strerror(errno));
   
   // Build latency frame and calculate latency (sender)
   build_latency_frame(&f,connection,&tlatency,localhost,1);
   
   // Receive latency frame with latency (receiver)
   if(send_frame(sock,&f) == -1)
               fprintf(stderr, "measure_latency: send(): %s\n",strerror(errno));
}


#endif
