#ifndef NET_H
#define NET_H

/* Shell commands */
void net_info(void);                 /* "net"            MAC / link / addresses */
void net_ping(const char* target);   /* "ping <host|ip>"                        */
void net_lspci(void);                /* "lspci"          list PCI devices       */

#endif