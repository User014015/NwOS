#include "net.h"
#include "e1000.h"
#include "timer.h"
#include "pci.h"

typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned int       u32;
typedef unsigned long long u64;

void print(const char* text);
void putchar_os(char c);
void print_int(int number);
void print_error(const char* text);
void print_success(const char* text);

static u8  my_mac[6];
static u32 my_ip   = 0x0A00020FU;      /* 10.0.2.15  */
static u32 my_mask = 0xFFFFFF00U;      /* 255.255.255.0 */
static u32 my_gw   = 0x0A000202U;      /* 10.0.2.2   */
static u32 my_dns  = 0x0A000203U;      /* 10.0.2.3   */

static int net_state = 0;              /* 0 = not initialised, 1 = ready */

static u8  rxf[2048];                  /* incoming frame            */
static u8  txf[1536];                  /* outgoing Ethernet frame   */
static u8  ipb[1500];                  /* outgoing IP packet        */
static u8  udpb[600];                  /* outgoing UDP datagram     */
static u8  icmpb[128];                 /* outgoing ping (ours)      */
static u8  rplb[128];                  /* outgoing echo reply (peer's ping) */

static u16 ip_id = 1;
static int in_poll = 0;

static const u8 bcast_mac[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

static u16 rd16(const u8* p) { return (u16)(((u16)p[0] << 8) | p[1]); }

static u32 rd32(const u8* p)
{
    return ((u32)p[0] << 24) | ((u32)p[1] << 16) | ((u32)p[2] << 8) | p[3];
}

static void wr16(u8* p, u16 v) { p[0] = (u8)(v >> 8); p[1] = (u8)v; }

static void wr32(u8* p, u32 v)
{
    p[0] = (u8)(v >> 24); p[1] = (u8)(v >> 16); p[2] = (u8)(v >> 8); p[3] = (u8)v;
}

static void n_copy(u8* d, const u8* s, u32 n)
{
    for (u32 i = 0; i < n; i++)
        d[i] = s[i];
}

static void n_zero(u8* d, u32 n)
{
    for (u32 i = 0; i < n; i++)
        d[i] = 0;
}

static u16 cksum(const u8* d, u32 n)
{
    u32 s = 0;

    while (n > 1)
    {
        s += (u32)(((u32)d[0] << 8) | d[1]);
        d += 2;
        n -= 2;
    }

    if (n)
        s += (u32)d[0] << 8;

    while (s >> 16)
        s = (s & 0xFFFF) + (s >> 16);

    return (u16)(~s & 0xFFFF);
}

static void print_hex(u32 v, int digits)
{
    const char hex[] = "0123456789ABCDEF";

    for (int i = digits - 1; i >= 0; i--)
        putchar_os(hex[(v >> (i * 4)) & 0xF]);
}

static void print_ip(u32 ip)
{
    print_int((int)((ip >> 24) & 255)); putchar_os('.');
    print_int((int)((ip >> 16) & 255)); putchar_os('.');
    print_int((int)((ip >> 8) & 255));  putchar_os('.');
    print_int((int)(ip & 255));
}

static void print_mac(const u8* m)
{
    for (int i = 0; i < 6; i++)
    {
        print_hex(m[i], 2);
        if (i < 5)
            putchar_os(':');
    }
}

static void print_ms(u64 us)
{
    print_int((int)(us / 1000));
    putchar_os('.');
    print_int((int)((us % 1000) / 100));
}


static void net_poll(void);

static int eth_send(const u8* dmac, u16 type, const u8* payload, u32 len)
{
    if (len > 1500)
        return -1;

    n_copy(txf, dmac, 6);
    n_copy(txf + 6, my_mac, 6);
    wr16(txf + 12, type);
    n_copy(txf + 14, payload, len);

    u32 total = 14 + len;

    if (total < 60)
    {
        n_zero(txf + total, 60 - total);
        total = 60;
    }

    return e1000_send(txf, total);
}

#define ARP_N 4

static u32 arp_ip[ARP_N];
static u8  arp_mac[ARP_N][6];
static u8  arp_ok[ARP_N];
static int arp_next = 0;

static void arp_learn(u32 ip, const u8* mac)
{
    for (int i = 0; i < ARP_N; i++)
    {
        if (arp_ok[i] && arp_ip[i] == ip)
        {
            n_copy(arp_mac[i], mac, 6);
            return;
        }
    }

    int slot = arp_next;
    arp_next = (arp_next + 1) % ARP_N;

    arp_ip[slot] = ip;
    arp_ok[slot] = 1;
    n_copy(arp_mac[slot], mac, 6);
}

static int arp_find(u32 ip, u8* mac)
{
    for (int i = 0; i < ARP_N; i++)
    {
        if (arp_ok[i] && arp_ip[i] == ip)
        {
            n_copy(mac, arp_mac[i], 6);
            return 1;
        }
    }

    return 0;
}

static void arp_send(u16 op, const u8* tha, u32 tpa, const u8* eth_dst)
{
    u8 p[28];

    wr16(p, 1);                 /* Ethernet */
    wr16(p + 2, 0x0800);        /* IPv4     */
    p[4] = 6;
    p[5] = 4;
    wr16(p + 6, op);
    n_copy(p + 8, my_mac, 6);
    wr32(p + 14, my_ip);
    n_copy(p + 18, tha, 6);
    wr32(p + 24, tpa);

    eth_send(eth_dst, 0x0806, p, 28);
}

static int arp_resolve(u32 ip, u8* mac)
{
    static const u8 zero_mac[6] = { 0, 0, 0, 0, 0, 0 };

    if (arp_find(ip, mac))
        return 1;

    for (int attempt = 0; attempt < 3; attempt++)
    {
        arp_send(1, zero_mac, ip, bcast_mac);

        u64 t = timer_ms();

        while (timer_ms() - t < 1000)
        {
            net_poll();

            if (arp_find(ip, mac))
                return 1;

            __asm__ volatile ("pause");
        }
    }

    return 0;
}

static void handle_arp(const u8* a, u32 len)
{
    if (len < 28)
        return;

    if (rd16(a) != 1 || rd16(a + 2) != 0x0800 || a[4] != 6 || a[5] != 4)
        return;

    u16 op = rd16(a + 6);
    u8  sha[6];
    n_copy(sha, a + 8, 6);
    u32 spa = rd32(a + 14);
    u32 tpa = rd32(a + 24);

    if (spa != 0)
        arp_learn(spa, sha);

    if (op == 1 && tpa == my_ip)
        arp_send(2, sha, spa, sha);
}


static int ip_send_mac(const u8* dmac, u32 dst, u8 proto, const u8* pl, u32 len)
{
    if (len > sizeof(ipb) - 20)
        return -1;

    ipb[0] = 0x45;
    ipb[1] = 0;
    wr16(ipb + 2, (u16)(20 + len));
    wr16(ipb + 4, ip_id++);
    wr16(ipb + 6, 0x4000);              /* don't fragment */
    ipb[8] = 64;                        /* TTL            */
    ipb[9] = proto;
    wr16(ipb + 10, 0);
    wr32(ipb + 12, my_ip);
    wr32(ipb + 16, dst);
    wr16(ipb + 10, cksum(ipb, 20));
    n_copy(ipb + 20, pl, len);

    return eth_send(dmac, 0x0800, ipb, 20 + len);
}

static int ip_send(u32 dst, u8 proto, const u8* pl, u32 len)
{
    u8 mac[6];

    if (dst == 0xFFFFFFFFU)
    {
        n_copy(mac, bcast_mac, 6);
    }
    else
    {
        u32 next = ((dst & my_mask) == (my_ip & my_mask)) ? dst : my_gw;

        if (!arp_resolve(next, mac))
            return -1;
    }

    return ip_send_mac(mac, dst, proto, pl, len);
}

#define PING_ID 0x4E57                  /* "NW" */

static int ping_waiting = 0;
static u16 ping_seq = 0;
static int ping_got = 0;
static u32 ping_from = 0;
static u8  ping_ttl = 0;
static u64 ping_rx_us = 0;
static int ping_err = 0;                /* (type << 8) | code of an ICMP error */

static void handle_icmp(const u8* ip, const u8* pl, u32 pn, u32 src, const u8* smac)
{
    if (pn < 8)
        return;

    u8 type = pl[0];

    if (type == 8)                      /* somebody pings us */
    {
        if (pn > sizeof(rplb) || cksum(pl, pn) != 0)
            return;

        n_copy(rplb, pl, pn);
        rplb[0] = 0;
        wr16(rplb + 2, 0);
        wr16(rplb + 2, cksum(rplb, pn));
        ip_send_mac(smac, src, 1, rplb, pn);
    }
    else if (type == 0)                 /* echo reply */
    {
        if (ping_waiting && rd16(pl + 4) == PING_ID && rd16(pl + 6) == ping_seq)
        {
            ping_got = 1;
            ping_from = src;
            ping_ttl = ip[8];
            ping_rx_us = timer_us();
        }
    }
    else if (type == 3 || type == 11 || type == 12)   /* error about our packet */
    {
        const u8* orig = pl + 8;
        u32 left = pn - 8;

        if (left >= 28 && (orig[0] >> 4) == 4 && orig[9] == 1)
        {
            u32 ihl2 = (u32)(orig[0] & 15) * 4;

            if (ihl2 >= 20 && left >= ihl2 + 8 &&
                rd16(orig + ihl2 + 4) == PING_ID &&
                rd16(orig + ihl2 + 6) == ping_seq && ping_waiting)
            {
                ping_err = (type << 8) | pl[1];
            }
        }
    }
}

/* ---- UDP / DNS ---- */

static u8  dns_buf[512];
static u32 dns_len = 0;
static int dns_got = 0;
static int dns_wait = 0;
static u16 dns_sport = 0;

static int udp_send(u32 dst, u16 sport, u16 dport, const u8* data, u32 len)
{
    if (len > sizeof(udpb) - 8)
        return -1;

    wr16(udpb, sport);
    wr16(udpb + 2, dport);
    wr16(udpb + 4, (u16)(8 + len));
    wr16(udpb + 6, 0);                  /* checksum optional over IPv4 */
    n_copy(udpb + 8, data, len);

    return ip_send(dst, 17, udpb, 8 + len);
}

static void handle_udp(const u8* pl, u32 pn)
{
    if (pn < 8)
        return;

    u16 sport = rd16(pl);
    u16 dport = rd16(pl + 2);
    u32 ulen  = rd16(pl + 4);

    if (ulen < 8 || ulen > pn)
        return;

    if (dns_wait && !dns_got && sport == 53 && dport == dns_sport)
    {
        u32 n = ulen - 8;

        if (n > sizeof(dns_buf))
            n = sizeof(dns_buf);

        n_copy(dns_buf, pl + 8, n);
        dns_len = n;
        dns_got = 1;
    }
}

static void handle_ip(const u8* p, u32 len, const u8* smac)
{
    if (len < 20 || (p[0] >> 4) != 4)
        return;

    u32 ihl = (u32)(p[0] & 15) * 4;

    if (ihl < 20 || ihl > len)
        return;

    u32 tot = rd16(p + 2);

    if (tot < ihl || tot > len)
        return;

    if (cksum(p, ihl) != 0)
        return;

    if (rd16(p + 6) & 0x3FFF)
        return;

    u32 src = rd32(p + 12);
    u32 dst = rd32(p + 16);

    if (dst != my_ip && dst != 0xFFFFFFFFU)
        return;

    if (p[9] == 1)
        handle_icmp(p, p + ihl, tot - ihl, src, smac);
    else if (p[9] == 17)
        handle_udp(p + ihl, tot - ihl);
}

static void handle_frame(const u8* f, u32 len)
{
    if (len < 14)
        return;

    u16 type = rd16(f + 12);

    if (type == 0x0806)
        handle_arp(f + 14, len - 14);
    else if (type == 0x0800)
        handle_ip(f + 14, len - 14, f + 6);
}

static void net_poll(void)
{
    if (in_poll)
        return;

    in_poll = 1;

    for (int i = 0; i < 16; i++)
    {
        int n = e1000_recv(rxf, sizeof(rxf));

        if (n <= 0)
            break;

        handle_frame(rxf, (u32)n);
    }

    in_poll = 0;
}

static u32 n_len(const char* s)
{
    u32 n = 0;
    while (s[n])
        n++;
    return n;
}

/* Builds an A/IN query for `name`. Returns its length, or -1 if the name is bad. */
static int dns_build(const char* name, u16 id, u8* q)
{
    u32 nlen = n_len(name);

    if (nlen == 0 || nlen > 253)
        return -1;

    wr16(q, id);
    wr16(q + 2, 0x0100);                /* recursion desired */
    wr16(q + 4, 1);                     /* one question      */
    wr16(q + 6, 0);
    wr16(q + 8, 0);
    wr16(q + 10, 0);

    u32 pos = 12;
    u32 i = 0;

    while (i < nlen)
    {
        u32 label_at = pos++;
        u32 label_len = 0;

        while (i < nlen && name[i] != '.')
        {
            if (label_len >= 63)
                return -1;

            q[pos++] = (u8)name[i++];
            label_len++;
        }

        if (label_len == 0)
            return -1;                  /* "..", leading dot */

        q[label_at] = (u8)label_len;

        if (i < nlen)
            i++;                        /* skip the dot */
    }

    q[pos++] = 0;
    wr16(q + pos, 1);                   /* type A     */
    wr16(q + pos + 2, 1);               /* class IN   */

    return (int)(pos + 4);
}

/* Returns the offset just past the (possibly compressed) name at `pos`, or -1. */
static int dns_skip_name(const u8* b, u32 len, u32 pos)
{
    while (pos < len)
    {
        u8 c = b[pos];

        if (c == 0)
            return (int)(pos + 1);

        if ((c & 0xC0) == 0xC0)
            return (pos + 2 <= len) ? (int)(pos + 2) : -1;

        pos += 1u + c;
    }

    return -1;
}

/* Result: 1 = got an address, 0 = no such name / no A record, -1 = malformed. */
static int dns_parse(const u8* b, u32 len, u16 id, u32* ip_out)
{
    if (len < 12 || rd16(b) != id)
        return -1;

    u16 flags = rd16(b + 2);

    if (!(flags & 0x8000))
        return -1;

    if ((flags & 0x000F) != 0)
        return 0;                       /* NXDOMAIN, SERVFAIL, ... */

    u32 qd = rd16(b + 4);
    u32 an = rd16(b + 6);
    u32 pos = 12;

    for (u32 i = 0; i < qd; i++)
    {
        int p = dns_skip_name(b, len, pos);

        if (p < 0 || (u32)p + 4 > len)
            return -1;

        pos = (u32)p + 4;
    }

    for (u32 i = 0; i < an; i++)
    {
        int p = dns_skip_name(b, len, pos);

        if (p < 0 || (u32)p + 10 > len)
            return -1;

        u32 at = (u32)p;
        u16 type = rd16(b + at);
        u16 cls  = rd16(b + at + 2);
        u32 rdl  = rd16(b + at + 8);

        at += 10;

        if (at + rdl > len)
            return -1;

        if (type == 1 && cls == 1 && rdl == 4)
        {
            *ip_out = rd32(b + at);
            return 1;
        }

        pos = at + rdl;                 /* CNAME and friends: keep looking */
    }

    return 0;
}

static int dns_resolve(const char* name, u32* ip_out)
{
    u8 q[300];

    for (int attempt = 0; attempt < 3; attempt++)
    {
        u16 id = (u16)(timer_us() & 0xFFFF) ^ (u16)(attempt * 0x1111);
        int qlen = dns_build(name, id, q);

        if (qlen < 0)
            return -1;

        dns_sport = (u16)(49152 + (id & 0x3FF));
        dns_got = 0;
        dns_wait = 1;

        if (udp_send(my_dns, dns_sport, 53, q, (u32)qlen) < 0)
        {
            dns_wait = 0;
            continue;
        }

        u64 t = timer_ms();

        while (!dns_got && timer_ms() - t < 3000)
        {
            net_poll();
            __asm__ volatile ("pause");
        }

        dns_wait = 0;

        if (!dns_got)
            continue;

        int r = dns_parse(dns_buf, dns_len, id, ip_out);

        if (r >= 0)
            return r;
    }

    return -2;
}

static int net_ensure(void)
{
    if (net_state == 1)
        return 1;

    timer_init();

    int r = e1000_init();

    if (r == E1000_ERR_NO_DEVICE)
    {
        print_error("No supported network card found.\n");
        print("Start QEMU with:  -nic user,model=e1000\n");
        print("(type 'lspci' to see what is attached)\n");
        return 0;
    }

    if (r != 0)
    {
        print_error("Network card initialisation failed (code ");
        print_int(r);
        print(")\n");
        return 0;
    }

    n_copy(my_mac, e1000_mac(), 6);

    u64 t = timer_ms();

    while (!e1000_link_up() && timer_ms() - t < 5000)
        __asm__ volatile ("pause");

    if (!e1000_link_up())
    {
        print_error("Network link is down.\n");
        return 0;
    }

    for (int i = 0; i < ARP_N; i++)
        arp_ok[i] = 0;

    net_state = 1;

    print("e1000: ");
    print_mac(my_mac);
    print(", link up\n");

    return 1;
}

void net_info(void)
{
    if (!net_ensure())
        return;

    print("\n  NIC:   Intel e1000\n");
    print("  MAC:   "); print_mac(my_mac); print("\n");
    print("  Link:  "); print(e1000_link_up() ? "up" : "down"); print("\n");
    print("  IP:    "); print_ip(my_ip); print("\n");
    print("  Mask:  "); print_ip(my_mask); print("\n");
    print("  GW:    "); print_ip(my_gw); print("\n");
    print("  DNS:   "); print_ip(my_dns); print("\n\n");
}

static int parse_ip(const char* s, u32* out)
{
    u32 value = 0;

    for (int part = 0; part < 4; part++)
    {
        if (*s < '0' || *s > '9')
            return 0;

        u32 octet = 0;
        int digits = 0;

        while (*s >= '0' && *s <= '9')
        {
            octet = octet * 10 + (u32)(*s - '0');

            if (++digits > 3 || octet > 255)
                return 0;

            s++;
        }

        value = (value << 8) | octet;

        if (part < 3)
        {
            if (*s != '.')
                return 0;
            s++;
        }
    }

    if (*s != '\0')
        return 0;

    *out = value;
    return 1;
}

void net_ping(const char* arg)
{
    char host[128];
    u32 i = 0;

    while (*arg == ' ')
        arg++;

    while (*arg && *arg != ' ' && i < sizeof(host) - 1)
        host[i++] = *arg++;

    host[i] = '\0';

    if (host[0] == '\0')
    {
        print_error("Usage: ping <host or IP>\n");
        return;
    }

    if (!net_ensure())
        return;

    u32 target;

    if (!parse_ip(host, &target))
    {
        print("Resolving ");
        print(host);
        print("...\n");

        int r = dns_resolve(host, &target);

        if (r == 1)
        {
            print(host);
            print(" is ");
            print_ip(target);
            print("\n");
        }
        else if (r == 0)
        {
            print_error("Host not found: ");
            print_error(host);
            print("\n");
            return;
        }
        else if (r == -1)
        {
            print_error("Invalid host name.\n");
            return;
        }
        else
        {
            print_error("DNS server did not answer (");
            print_ip(my_dns);
            print(").\n");
            return;
        }
    }

    print("\nPinging ");
    print_ip(target);
    print(" with 32 bytes of data:\n");

    int received = 0;

    for (int n = 0; n < 4; n++)
    {
        ping_seq++;
        ping_got = 0;
        ping_err = 0;
        ping_waiting = 1;

        icmpb[0] = 8;
        icmpb[1] = 0;
        wr16(icmpb + 2, 0);
        wr16(icmpb + 4, PING_ID);
        wr16(icmpb + 6, ping_seq);

        for (int k = 0; k < 32; k++)
            icmpb[8 + k] = (u8)('a' + (k % 23));

        wr16(icmpb + 2, cksum(icmpb, 40));

        u64 sent_us = timer_us();

        if (ip_send(target, 1, icmpb, 40) < 0)
        {
            ping_waiting = 0;
            print_error("No ARP reply: that address is not on the network.\n");
            return;
        }

        u64 t = timer_ms();

        while (!ping_got && !ping_err && timer_ms() - t < 2000)
        {
            net_poll();
            __asm__ volatile ("pause");
        }

        ping_waiting = 0;

        if (ping_got)
        {
            received++;
            print("Reply from ");
            print_ip(ping_from);
            print(": bytes=32 time=");
            print_ms(ping_rx_us >= sent_us ? ping_rx_us - sent_us : 0);
            print("ms TTL=");
            print_int(ping_ttl);
            print("\n");
        }
        else if (ping_err)
        {
            print_error("Destination unreachable (ICMP type ");
            print_int(ping_err >> 8);
            print(", code ");
            print_int(ping_err & 0xFF);
            print(").\n");
        }
        else
        {
            print_error("Request timed out.\n");
        }

        while (timer_us() - sent_us < 1000000ull)
        {
            net_poll();
            __asm__ volatile ("pause");
        }
    }

    print("\nSent 4, received ");
    print_int(received);
    print(", lost ");
    print_int(4 - received);
    print("\n\n");

    if (received == 0)
        print("No replies. 'ping 10.0.2.2' tests the virtual gateway.\n");
}

void net_lspci(void)
{
    pci_dev_t list[32];
    int n = pci_scan(list, 32);

    print("\n");

    for (int i = 0; i < n; i++)
    {
        print_hex(list[i].bus, 2);
        putchar_os(':');
        print_hex(list[i].dev, 2);
        putchar_os('.');
        print_hex(list[i].func, 1);
        print("  ");
        print_hex(list[i].vendor, 4);
        putchar_os(':');
        print_hex(list[i].device, 4);
        print("  ");

        switch (list[i].class_code)
        {
            case 0x01: print("storage");   break;
            case 0x02: print("network");   break;
            case 0x03: print("display");   break;
            case 0x06: print("bridge");    break;
            default:
                print("class ");
                print_hex(list[i].class_code, 2);
                break;
        }

        print("\n");
    }

    print("\n");
}