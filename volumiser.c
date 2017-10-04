/*
 * tcpserver.c - A simple TCP echo server
 * usage: tcpserver <port>
 */

#include <stdio.h>
#include <stdlib.h>
#include <alsa/asoundlib.h>

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFSIZE 1024

#if 0
/*
 * Structs exported from in.h
 */

/* Internet address */
struct in_addr {
  unsigned int s_addr;
};

/* Internet style socket address */
struct sockaddr_in  {
  unsigned short int sin_family; /* Address family */
  unsigned short int sin_port;   /* Port number */
  struct in_addr sin_addr;	 /* IP address */
  unsigned char sin_zero[...];   /* Pad to size of 'struct sockaddr' */
};

/*
 * Struct exported from netdb.h
 */

/* Domain name service (DNS) host entry */
struct hostent {
  char    *h_name;        /* official name of host */
  char    **h_aliases;    /* alias list */
  int     h_addrtype;     /* host address type */
  int     h_length;       /* length of address */
  char    **h_addr_list;  /* list of addresses */
}
#endif

/*
 * error - wrapper for perror
 */
void error(char *msg) {
  perror(msg);
  exit(1);
}

int send_volume (char * device, int fd)
{
  int i, j;
  int err;
  unsigned int sample_rate = 48000;
  unsigned long frames;
  short * buf;
  short min, max, amp=0;
  snd_pcm_t *capture_handle;
  snd_pcm_hw_params_t *hw_params;
  /*  int bufsize = 2 * (16/8) * 44100 * 1;  */
  int bufsize = 128;

  int bytes;
  char out_buf[40];

  if((buf = calloc(bufsize, 2 * (sizeof (short)))) == 0) {
    fprintf (stderr, "cannot allocate buffer (size %d)",
             bufsize);
    exit (1);
  }

  if ((err = snd_pcm_open (&capture_handle, device, SND_PCM_STREAM_CAPTURE, 0)) < 0) {
    fprintf (stderr, "cannot open audio device %s (%s)\n",
             device,
             snd_strerror (err));
    exit (1);
  }
  fprintf(stderr, "line %d\n", __LINE__);

  if ((err = snd_pcm_hw_params_malloc (&hw_params)) < 0) {
    fprintf (stderr, "cannot allocate hardware parameter structure (%s)\n",
             snd_strerror (err));
    exit (1);
  }
  fprintf(stderr, "line %d\n", __LINE__);
  if ((err = snd_pcm_hw_params_any (capture_handle, hw_params)) < 0) {
    fprintf (stderr, "cannot initialize hardware parameter structure (%s)\n",
             snd_strerror (err));
    exit (1);
  }
    fprintf(stderr, "line %d\n", __LINE__);

  if ((err = snd_pcm_hw_params_set_access (capture_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
    fprintf (stderr, "cannot set access type (%s)\n",
             snd_strerror (err));
    exit (1);
  }
  fprintf(stderr, "line %d\n", __LINE__);
  if ((err = snd_pcm_hw_params_set_format (capture_handle, hw_params, SND_PCM_FORMAT_S16_LE)) < 0) {
    fprintf (stderr, "cannot set sample format (%s)\n",
             snd_strerror (err));
    exit (1);
  }
  fprintf(stderr, "line %d\n", __LINE__);
  if ((err = snd_pcm_hw_params_set_rate_near (capture_handle, hw_params, &sample_rate , 0)) < 0) {
    fprintf (stderr, "cannot set sample rate (%s)\n",
             snd_strerror (err));
    exit (1);
  }
  fprintf(stderr, "line %d rate %d\n", __LINE__, sample_rate);
  if ((err = snd_pcm_hw_params_set_channels (capture_handle, hw_params, 2)) < 0) {
    fprintf (stderr, "cannot set channel count (%s)\n",
             snd_strerror (err));
    exit (1);
  }
  fprintf(stderr, "line %d\n", __LINE__);
  if ((err = snd_pcm_hw_params (capture_handle, hw_params)) < 0) {
    fprintf (stderr, "cannot set parameters (%s)\n",
             snd_strerror (err));
    exit (1);
  }
  fprintf(stderr, "line %d\n", __LINE__);
  snd_pcm_hw_params_free (hw_params);

  if ((err = snd_pcm_prepare (capture_handle)) < 0) {
    fprintf (stderr, "cannot prepare audio interface for use (%s)\n",
             snd_strerror (err));
    exit (1);
  }
  fprintf(stderr, "line %d\n", __LINE__);
  err = 0;
  while(frames >= 0) {
    min = 32767;
    max = -32768;
    if ((frames = snd_pcm_readi (capture_handle, buf, bufsize)) != bufsize) {
      fprintf (stderr, "read from audio interface failed (%s)\n",
               snd_strerror (frames));
    } else {
      for(j = 0; j < bufsize; j++) {
        short val = buf[j];
        if(val > max) max = val;
        if(val < min) min = val;
      }
#define SMOOTH (2<<4)
      amp = (SMOOTH-1)*(amp/SMOOTH) + (max-min)/SMOOTH;
      if((i % 100) == 0) {
        fprintf(stderr, " hey %*s\n",  amp/500, "*");
        bytes = sprintf(out_buf, "%d\n", amp);
        write(fd, out_buf, bytes);
        i=1;
      } else {
        i++;
      }
    }
  }

  fprintf(stderr, "line %d err %d\n", __LINE__, err);
  free(buf);
  snd_pcm_close (capture_handle);
  exit (0);
}

int main(int argc, char **argv) {
  int parentfd; /* parent socket */
  int childfd; /* child socket */
  int portno; /* port to listen on */
  int clientlen; /* byte size of client's address */
  struct sockaddr_in serveraddr; /* server's addr */
  struct sockaddr_in clientaddr; /* client addr */
  struct hostent *hostp; /* client host info */
  char buf[BUFSIZE]; /* message buffer */
  char *hostaddrp; /* dotted decimal host addr string */
  int optval; /* flag value for setsockopt */
  int n; /* message byte size */

  /*
   * check command line arguments
   */
  if (argc != 3) {
    fprintf(stderr, "usage: %s <port> <device>\n", argv[0]);
    exit(1);
  }
  portno = atoi(argv[1]);

  /*
   * socket: create the parent socket
   */
  parentfd = socket(AF_INET, SOCK_STREAM, 0);
  if (parentfd < 0)
    error("ERROR opening socket");

  /* setsockopt: Handy debugging trick that lets
   * us rerun the server immediately after we kill it;
   * otherwise we have to wait about 20 secs.
   * Eliminates "ERROR on binding: Address already in use" error.
   */
  optval = 1;
  setsockopt(parentfd, SOL_SOCKET, SO_REUSEADDR,
	     (const void *)&optval , sizeof(int));

  /*
   * build the server's Internet address
   */
  bzero((char *) &serveraddr, sizeof(serveraddr));

  /* this is an Internet address */
  serveraddr.sin_family = AF_INET;

  /* let the system figure out our IP address */
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);

  /* this is the port we will listen on */
  serveraddr.sin_port = htons((unsigned short)portno);

  /*
   * bind: associate the parent socket with a port
   */
  if (bind(parentfd, (struct sockaddr *) &serveraddr,
	   sizeof(serveraddr)) < 0)
    error("ERROR on binding");

  /*
   * listen: make this socket ready to accept connection requests
   */
  if (listen(parentfd, 5) < 0) /* allow 5 requests to queue up */
    error("ERROR on listen");

  /*
   * main loop: wait for a connection request, echo input line,
   * then close connection.
   */
  clientlen = sizeof(clientaddr);
  while (1) {

    /*
     * accept: wait for a connection request
     */
    childfd = accept(parentfd, (struct sockaddr *) &clientaddr, &clientlen);
    if (childfd < 0)
      error("ERROR on accept");

    /*
     * gethostbyaddr: determine who sent the message
     */
    hostp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr,
			  sizeof(clientaddr.sin_addr.s_addr), AF_INET);
    if (hostp == NULL)
      error("ERROR on gethostbyaddr");
    hostaddrp = inet_ntoa(clientaddr.sin_addr);
    if (hostaddrp == NULL)
      error("ERROR on inet_ntoa\n");
    printf("server established connection with %s (%s)\n",
	   hostp->h_name, hostaddrp);

    /*
     * read: read input string from the client
     */
#if 0
    bzero(buf, BUFSIZE);
    n = read(childfd, buf, BUFSIZE);
    if (n < 0)
      error("ERROR reading from socket");
    printf("server received %d bytes: %s", n, buf);
#endif
    /*
     * write: echo the input string back to the client
     */

    send_volume(argv[2], childfd);
    close(childfd);
  }
}
