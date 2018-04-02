/**
 * C helper file for testing whether `lib.rs` actually compiles
 */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <net/if.h>
#include <linux/if_tun.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <fcntl.h>

int tun_alloc(char *dev, int flags)
{

  struct ifreq ifr;
  int fd, err;
  char *clonedev = "/dev/net/tun";

  /* Arguments taken by the function:
   *
   * char *dev: the name of an interface (or '\0'). MUST have enough
   *   space to hold the interface name if '\0' is passed
   * int flags: interface flags (eg, IFF_TUN etc.)
   */

  /* open the clone device */
  if ((fd = open(clonedev, O_RDWR)) < 0) {
    return fd;
  }

  /* preparation of the struct ifr, of type "struct ifreq" */
  memset(&ifr, 0, sizeof(ifr));

  ifr.ifr_flags = flags; /* IFF_TUN or IFF_TAP, plus maybe IFF_NO_PI */

  if (*dev) {
    /* if a device name was specified, put it in the structure; otherwise,
     * the kernel will try to allocate the "next" device of the
     * specified type */
    strncpy(ifr.ifr_name, dev, IFNAMSIZ);
  }

  /* try to create the device */
  if ((err = ioctl(fd, TUNSETIFF, (void *) &ifr)) < 0) {
    close(fd);
    return err;
  }

  /* if the operation was successful, write back the name of the
   * interface to the variable "dev", so the caller can know
   * it. Note that the caller MUST reserve space in *dev (see calling
   * code below) */
  strcpy(dev, ifr.ifr_name);

  /*
   * Setup a non blocking call
   */
  //flags = fcntl(fd, F_GETFL, 0);
  //fcntl(fd, F_SETFL, flags | O_NONBLOCK);
  /* this is the special file descriptor that the caller will use to talk
   * with the virtual interface */
  return fd;
}

int tun_fd;
char tun_name[IFNAMSIZ];
char tun_buffer[1500];
fd_set set;
struct timeval timeout;
int rv;

/*
 // Stubs for the external firewall calls
 void packet_in(uint32_t src_addr, uint16_t src_port,
 uint32_t dest_addr, uint16_t dest_port,
 uint16_t payload_len, void *payload);

 void packet_out(uint32_t src_addr, uint16_t src_port,
 uint32_t dest_addr, uint16_t dest_port,
 uint16_t payload_len, void *payload);
 */

/**
 * A helper define to make this look more like an actual seL4 file
 */
typedef uint32_t seL4_Word;

extern void client_mac(uint8_t *b1, uint8_t *b2, uint8_t *b3, uint8_t *b4,
    uint8_t *b5, uint8_t *b6);
extern int client_tx(int len);
extern int client_rx(int *len);
extern void ethdriver_has_data_callback(seL4_Word badge);

/**
 * Note: this code is normally autogenerated during seL4 build
 */
struct
{
  char content[4096];
} from_ethdriver_data;

//volatile void * ethdriver_buf = (volatile void *) &from_ethdriver_data;
void * ethdriver_buf = (void *) &from_ethdriver_data;

struct
{
  char content[4096];
} to_client_1_data;

//volatile void * client_buf_1 = (volatile void *) &to_client_1_data;
void * client_buf_1 = (void *) &to_client_1_data;

void *client_buf(seL4_Word client_id)
{
  switch (client_id) {
    case 1:
      return (void *) client_buf_1;
    default:
      return NULL;
  }
}

void client_emit_1(void)
{
  printf("Client emit 1: calling seL4_signal()\n");
}

void client_emit(unsigned int badge)
{
  // here is normally a array of functions:
  //static void (*lookup[])(void) {
  //  [1] = client_emit_1,
  //};
  //lookup[badge]();
  if (badge == 1) {
    client_emit_1();
  };
}
/**
 * END OF AUTOGENERATED CODE
 */

/**
 * Dummy version
 * Normally sends `len` data from `ethdriver_buf`
 * Returns -1 in case of an error, and probably 0 if all OK
 */
int ethdriver_tx(int len)
{
  printf("C Attempt to write %i bytes\n", len);
  memcpy(tun_buffer, ethdriver_buf, len);
  len = write(tun_fd, tun_buffer, len);
  if (len < 0) {
    perror("C Writing to interface");
    close(tun_fd);
    exit(1);
  } else {
    printf("C Wrote %i bytes\n", len);
  }
  return 0;
}

char packet_bytes[] = { 2, 0, 0, 0, 1, 1, 82, 84, 0, 0, 0, 0, 8, 0, 69, 0, 0,
    31, 0, 0, 64, 0, 64, 17, 47, 120, 192, 168, 69, 3, 192, 168, 69, 2, 175,
    211, 27, 57, 0, 11, 190, 19, 97, 97, 10 };

/**
 * Dummy version
 * Normally receives `len` data and returns -1 in case of an error, and probably 0 if all OK
 */
int ethdriver_rx(int* len)
{
  /* Note that "buffer" should be at least the MTU size of the interface, eg 1500 bytes */
  timeout.tv_sec = 3;  // 5s read/write timeout
  timeout.tv_usec = 0;

  printf("C Attemp to read\n");
  rv = select(tun_fd + 1, &set, NULL, NULL, &timeout);
  if (rv == -1) {
    perror("C select\n"); /* an error accured */
    return -1;
  } else {
    if (rv == 0) {
      printf("C timeout\n"); /* a timeout occured */
      return -1;
    } else {
      printf("C Reading data\n");
      *len = read(tun_fd, tun_buffer, sizeof(tun_buffer));
      printf("C read %i bytes\n", *len);
      memcpy(ethdriver_buf, tun_buffer, *len);
      return 0;
    }
  }

  /*
   printf("C Attemp to read\n");
   return -1;
   *len = read(tun_fd,tun_buffer,sizeof(tun_buffer));
   if(*len < 0) {
   perror("C Reading from interface");
   close(tun_fd);
   exit(1);
   } else {
   printf("C read %i bytes\n",*len);
   memcpy(ethdriver_buf, tun_buffer, *len);
   }
   return 0;
   */
}

/**
 * Dummy version
 * Normally returns the MAC address of the ethernet driver
 */
void ethdriver_mac(uint8_t *b1, uint8_t *b2, uint8_t *b3, uint8_t *b4,
    uint8_t *b5, uint8_t *b6)
{
  static uint8_t mac[] = { 0x02, 0x00, 0x00, 0x00, 0x01, 0x01 };
  *b1 = mac[0];
  *b2 = mac[1];
  *b3 = mac[2];
  *b4 = mac[3];
  *b5 = mac[4];
  *b6 = mac[5];
  /*
   printf("Hello from ethdriver_mac: ");
   printf("b1=%u, ", *b1);
   printf("b2=%u, ", *b2);
   printf("b3=%u, ", *b3);
   printf("b4=%u, ", *b4);
   printf("b5=%u, ", *b5);
   printf("b6=%u\n", *b6);
   */
}

/**
 * Main program
 */
int main()
{
  /* Connect to the device */
  strcpy(tun_name, "tap0");
  tun_fd = tun_alloc(tun_name, IFF_TAP | IFF_NO_PI | O_NONBLOCK); /* tun interface */

  if (tun_fd < 0) {
    perror("Allocating interface");
    exit(1);
  }

  FD_ZERO(&set); /* clear the set */
  FD_SET(tun_fd, &set); /* add our file descriptor to the set */

  printf("hello from C\n");
  memset(&to_client_1_data, 0, sizeof(to_client_1_data));
  memset(&from_ethdriver_data, 0, sizeof(from_ethdriver_data));

  uint8_t b1 = 11;
  uint8_t b2 = 22;
  uint8_t b3 = 33;
  uint8_t b4 = 44;
  uint8_t b5 = 55;
  uint8_t b6 = 66;

  /*
   client_mac(&b1, &b2, &b3, &b4, &b5, &b6);

   printf("Initializing buffer\n");
   from_ethdriver_data.content[0] = 41;
   from_ethdriver_data.content[1] = 44;
   from_ethdriver_data.content[2] = 47;
   from_ethdriver_data.content[3] = 55;
   from_ethdriver_data.content[4] = '\0';

   //char* c = (char*)client_buf_1;
   //printf("c = %u\n",*c);

   // show ethdriver data
   printf("C ethdriver_buf[0] = %u\n", from_ethdriver_data.content[0]);
   printf("C ethdriver_buf[1] = %u\n", from_ethdriver_data.content[1]);
   printf("C ethdriver_buf[2] = %u\n", from_ethdriver_data.content[2]);
   printf("C ethdriver_buf[3] = %u\n", from_ethdriver_data.content[3]);
   printf("C ethdriver_buf[4] = %u\n", from_ethdriver_data.content[4]);
   */

  int len = 0;
  int returnval = 0;
  //printf("client_tx returned %u bytes\n", client_tx(len));
  /*
   // play with pointers
   printf("C passing len = %i\n", len);
   printf("C len address = %p\n", &len);
   printf("C ethdriver_buf address = %p\n", ethdriver_buf);
   printf("C from_ethdriver_data address = %p\n", &from_ethdriver_data);
   */
/*  // client receive
  printf("Client receive call 1\n");
  returnval = client_rx(&len);
  printf("client_rx received %u bytes with return value %i\n", len, returnval);
*/
  /*
   printf("C client_buf[0] = %u\n", to_client_1_data.content[0]);
   printf("C client_buf[1] = %u\n", to_client_1_data.content[1]);
   printf("C client_buf[2] = %u\n", to_client_1_data.content[2]);
   printf("C client_buf[3] = %u\n", to_client_1_data.content[3]);
   printf("C client_buf[4] = %u\n", to_client_1_data.content[4]);

   // data callback
   ethdriver_has_data_callback(66);
   */
  char* buf = (char*)client_buf(1);
   // client transmit
   for (int i = 0; i <= sizeof(packet_bytes); i++) {
     //to_client_1_data.content[i] = packet_bytes[i];
     buf[i] = packet_bytes[i];
     len = i;
   }

   returnval = client_tx(len);
   printf("client_tx transmitted %u bytes with return value %i\n", len,
   returnval);

   returnval = client_tx(len);
   printf("client_tx transmitted %u bytes with return value %i\n", len,
   returnval);

  /*
   printf("C ethdriver_buf[0] = %u\n", from_ethdriver_data.content[0]);
   printf("C ethdriver_buf[1] = %u\n", from_ethdriver_data.content[1]);
   printf("C ethdriver_buf[2] = %u\n", from_ethdriver_data.content[2]);
   printf("C ethdriver_buf[3] = %u\n", from_ethdriver_data.content[3]);
   printf("C ethdriver_buf[4] = %u\n", from_ethdriver_data.content[4]);

   client_mac(&b1, &b2, &b3, &b4, &b5, &b6);
   */

  printf("done\n");
}
