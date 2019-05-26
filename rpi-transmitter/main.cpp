#include	<unistd.h>
#include 	<stdio.h>
#include	<cstring>
#include	<signal.h>
#include	<stdlib.h>
#include	<stdint.h>
#include	"transmitter.h"
#include        <sys/types.h>
#include        <sys/socket.h>
#include	<arpa/inet.h>
#include        <netdb.h>
#include        <string>
#include        <thread>
#include        <unistd.h>
#include        <atomic>

#define PROGRAM_VERSION "0.2"
#define BUF_SIZE	1024

std::atomic<bool>	running;

void print_usage (void) {
fprintf(stderr,\
"\nsendiq -%s\n\
Usage:\nsendiq [-s Samplerate][-l] [-f Frequency] [-h Harmonic number] \n\
-s            SampleRate 10000-250000 \n\
-f float      central frequency Hz(50 kHz to 1500 MHz),\n\
-h            Use harmonic number n\n\
-?            help (this help).\n\
\n",\
PROGRAM_VERSION);

} /* end function print_usage */

static
void	terminate (int num) {

	running. store (false);
	fprintf (stderr,"Caught signal - Terminating %x\n",num);
}


int main (int argc, char* argv[]) {
bool	anyargs		= false;
float	frequency	= 434e6;
float	samplerate	= 96000;
int	harmonic	= 1;
int	currentArg;
int	port		= 8765;		// default port
	while (true) {
	   currentArg = getopt (argc, argv, "p:f:s:h:lt:");
	   if (currentArg == -1) {
	      if (anyargs)
	         break;
	      print_usage ();
	      exit (1);
	   }

	   fprintf (stderr, "option %c\n", (char)currentArg);
	   anyargs = true;	
	   switch ((char)currentArg) {
	      case 'p':		// port
	         port	= atoi (optarg);
	         break;

	      case 'f': // Frequency
	         frequency = atof (optarg);
	         break;

	      case 's': // SampleRate (Only needed in IQ mode)
	         samplerate = atoi (optarg);
	         break;

	      case 'h': // help
	         harmonic = atoi (optarg);
	         break;

	      default:
	         print_usage ();
	         exit (1);
	         break;
	   }/* end switch a */
	}/* end while getopt() */

	for (int i = 0; i < 64; i++) {
	   struct sigaction sa;

	   std::memset (&sa, 0, sizeof(sa));
	   sa. sa_handler = terminate;
	   sigaction (i, &sa, NULL);
	}

// Variables for writing a server.
/*
 *      1. Getting the address data structure.
 *      2. Opening a new socket.
 *      3. Bind to the socket.
 *      4. Listen to the socket.
 *      5. Accept Connection.
 *      6. Receive Data.
 *      7. Close Connection.
 */
        int socket_desc , client_sock , c , read_size;
        struct sockaddr_in server, client;

	int16_t   localBuffer	[BUF_SIZE];
	uint8_t   byteBuffer	[BUF_SIZE * 2];
//	Create socket
        socket_desc = socket (AF_INET , SOCK_STREAM , 0);
        if (socket_desc == -1) {
           fprintf (stderr, "Could not create socket, fatal");
	   exit (1);
        }

	fprintf (stderr, "Socket created");
        running. store (true);
//      Prepare the sockaddr_in structure
        server.sin_family = AF_INET;
        server.sin_addr.s_addr = INADDR_ANY;
        server.sin_port = htons (port);

//      Bind
        if (bind (socket_desc,
                  (struct sockaddr *)&server , sizeof(server)) < 0) {
//      print the error message
           perror ("bind failed. Error");
	   fprintf (stderr, "server cannot bind a socket, fatal\n");
	   exit (1);
        }

//      Listen
        listen (socket_desc , 3);
        while (running. load ()) {
	   fprintf (stderr, "I am now accepting connections ...\n");
//      Accept a new connection and return back the socket desciptor
           c = sizeof(struct sockaddr_in);

//      accept connection from an incoming client
           client_sock = accept (socket_desc,
                                 (struct sockaddr *)&client,
                                 (socklen_t*)&c);
           if (client_sock < 0) {
              perror ("accept failed");
              continue;
           }

	   fprintf (stderr, "Connection accepted,");
	   fprintf (stderr, "% s \n", inet_ntoa (client. sin_addr));
           Transmitter transmitter (frequency, samplerate, harmonic);
	   try {
              int16_t   amount;
              int status;
              while (running. load ()) {
                 amount = recv (client_sock, byteBuffer, BUF_SIZE * 2, 0);
	         if (amount <= 0)
	            throw (22);
                 for (int i = 0; i < amount / 2; i ++) {
	            uint8_t head = byteBuffer [2 * i];
	            uint8_t tail = byteBuffer [2 * i + 1];
                    int16_t t =
                        (((int16_t)head)<< 8)| (((int16_t)tail) & 0xFF);
                    localBuffer [i] = t;
                 }
	         transmitter. send (localBuffer, amount / 2);
              }
           }
           catch (int e) {}
	}
	close (socket_desc);
	return 1;
}	

