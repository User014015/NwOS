#ifndef E1000_H
#define E1000_H

#define E1000_ERR_NO_DEVICE   (-1)
#define E1000_ERR_BAD_BAR     (-2)
#define E1000_ERR_RESET       (-3)
#define E1000_ERR_NO_MAC      (-4)

int  e1000_init(void);                        /* 0 on success, else E1000_ERR_* */
int  e1000_link_up(void);                     /* 1 if the link is up           */
const unsigned char* e1000_mac(void);         /* 6 bytes                       */

int  e1000_send(const void* frame, unsigned int len);   /* 0 ok, <0 error      */
int  e1000_recv(void* buf, unsigned int max);           /* frame length, 0 = none */

#endif