#include "net.h"
#include "vga.h"
#include "serial.h"
#include "timer.h"
#include <stdint.h>

static inline void outb(uint16_t p,uint8_t v){ __asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p)); }
static inline uint8_t inb(uint16_t p){ uint8_t r; __asm__ volatile("inb %1,%0":"=a"(r):"Nd"(p)); return r; }
static inline void outl(uint16_t p,uint32_t v){ __asm__ volatile("outl %0,%1"::"a"(v),"Nd"(p)); }
static inline uint32_t inl(uint16_t p){ uint32_t r; __asm__ volatile("inl %1,%0":"=a"(r):"Nd"(p)); return r; }

static void kputs(const char *s){ vga_puts(s); serial_puts(s); }
static void kputc(char c){ char b[2]={c,0}; vga_puts(b); serial_puts(b); }
static void khex2(uint8_t v){ const char *d="0123456789ABCDEF"; kputc(d[v>>4]); kputc(d[v&0xF]); }
static void knum(uint32_t n){ char b[12]; int i=11; b[i]=0; if(!n){kputc('0');return;} while(n){b[--i]='0'+(n%10); n/=10;} kputs(b+i); }

#define NUM_RX 32
#define NUM_TX 32
struct rx_desc { uint64_t addr; uint16_t len; uint16_t csum; uint16_t status; uint16_t special; } __attribute__((packed));
struct tx_desc { uint64_t addr; uint16_t len; uint8_t cso; uint8_t cmd; uint8_t status; uint8_t css; uint16_t special; } __attribute__((packed));
static struct rx_desc rx[NUM_RX] __attribute__((aligned(16)));
static struct tx_desc tx[NUM_TX] __attribute__((aligned(16)));
static uint8_t rx_buf[NUM_RX][2048] __attribute__((aligned(16)));
static uint8_t tx_buf[NUM_TX][2048] __attribute__((aligned(16)));
static int rx_cur=0, tx_cur=0;

static volatile uint8_t *mmio = 0;
static uint8_t mac[6];
static uint8_t my_ip[4] = {10,0,2,15};
static int net_ok = 0;

static void wr32(uint32_t r,uint32_t v){ *(volatile uint32_t*)(mmio+r)=v; }
static uint32_t rd32(uint32_t r){ return *(volatile uint32_t*)(mmio+r); }
static void delay(void){ for(volatile int i=0;i<200000;i++); }

static uint32_t pci_read(uint8_t d,uint8_t o){ outl(0xCF8, 0x80000000u|(d<<11)|(o&0xFC)); return inl(0xCFC); }
static void pci_write(uint8_t d,uint8_t o,uint32_t v){ outl(0xCF8, 0x80000000u|(d<<11)|(o&0xFC)); outl(0xCFC, v); }

int net_install(void){
    int dev=-1;
    for (int d=0;d<32;d++){
        uint32_t id = pci_read((uint8_t)d, 0);
        if ((id&0xFFFF)==0x8086 && ((id>>16)&0xFFFF)==0x100E){ dev=d; break; }
    }
    if (dev<0){ serial_puts("[NET] e1000 not found\n"); return 0; }
    uint32_t bar0 = pci_read((uint8_t)dev, 0x10) & 0xFFFFFFF0u;
    pci_write((uint8_t)dev, 0x04, pci_read((uint8_t)dev,0x04) | 0x06);
    { uint32_t *pd = (uint32_t*)0x100000; uint32_t idx = bar0 >> 22; if (!(pd[idx] & 1)) { pd[idx] = (bar0 & 0xFFC00000u) | 0x83u; __asm__ volatile("mov %0, %%cr3" :: "r"((uint32_t)0x100000)); } }
    mmio = (volatile uint8_t*)bar0;

    wr32(0x0000, rd32(0x0000) | 0x04000000u);
    delay();
    uint32_t ctrl = rd32(0x0000);
    ctrl |= 0x40;
    ctrl &= ~0x80000000u;
    wr32(0x0000, ctrl);
    delay();
    wr32(0x00D0, 0);

    uint32_t lo = rd32(0x5400), hi = rd32(0x5404);
    mac[0]=lo&0xFF; mac[1]=(lo>>8)&0xFF; mac[2]=(lo>>16)&0xFF; mac[3]=(lo>>24)&0xFF;
    mac[4]=hi&0xFF; mac[5]=(hi>>8)&0xFF;

    for (int i=0;i<NUM_RX;i++){ rx[i].addr=(uint64_t)(uint32_t)rx_buf[i]; rx[i].status=0; }
    wr32(0x2800, (uint32_t)(uint64_t)rx); wr32(0x2804, 0);
    wr32(0x2808, NUM_RX*16); wr32(0x2810, 0); wr32(0x2818, NUM_RX-1);
    wr32(0x0100, (1u<<1)|(1u<<15)|(1u<<2));

    for (int i=0;i<NUM_TX;i++){ tx[i].addr=(uint64_t)(uint32_t)tx_buf[i]; tx[i].status=0; }
    wr32(0x3800, (uint32_t)(uint64_t)tx); wr32(0x3804, 0);
    wr32(0x3808, NUM_TX*16); wr32(0x3810, 0); wr32(0x3818, 0);
    wr32(0x0400, (1u<<1)|(1u<<3)|(15u<<4)|(64u<<12));

    net_ok = 1;
    serial_puts("[NET] e1000 up, IP 10.0.2.15\n");
    return 1;
}

static void tx_send(const uint8_t *pkt, int len){
    int i = tx_cur;
    for (int k=0;k<len;k++) tx_buf[i][k]=pkt[k];
    tx[i].len=(uint16_t)len;
    tx[i].cmd=0x03;
    tx[i].status=0;
    tx_cur=(i+1)%NUM_TX;
    wr32(0x3818, (uint32_t)tx_cur);
}

static int rx_recv(uint8_t *out, int max){
    int i = rx_cur;
    if (!(rx[i].status & 1)) return 0;
    int len = rx[i].len; if (len>max) len=max;
    for (int k=0;k<len;k++) out[k]=rx_buf[i][k];
    rx[i].status=0;
    rx_cur=(i+1)%NUM_RX;
    wr32(0x2818, (uint32_t)rx_cur);
    return len;
}

static uint16_t cksum(const uint8_t *p, int len){
    uint32_t s=0;
    for (int i=0;i<len-1;i+=2) s += (uint32_t)((p[i]<<8)|p[i+1]);
    if (len&1) s += (uint32_t)(p[len-1]<<8);
    while (s>>16) s = (s&0xFFFF)+(s>>16);
    return (uint16_t)~s;
}

static uint8_t peer_mac[6];
static uint8_t peer_ip[4];
static int peer_known = 0;
static volatile uint16_t ping_seq = 0;
static volatile int ping_done = 0;

static void eth_tx(const uint8_t *dst, uint16_t type, const uint8_t *pay, int len){
    uint8_t f[2048];
    for (int i=0;i<6;i++){ f[i]=dst[i]; f[6+i]=mac[i]; }
    f[12]=(uint8_t)(type>>8); f[13]=(uint8_t)type;
    for (int i=0;i<len;i++) f[14+i]=pay[i];
    tx_send(f, 14+len);
}

static void arp_send(uint16_t op, const uint8_t *dst_mac, const uint8_t *tgt_ip){
    uint8_t a[28];
    a[0]=0; a[1]=1; a[2]=8; a[3]=0; a[4]=0; a[5]=6; a[6]=0; a[7]=4;
    a[8]=(uint8_t)(op>>8); a[9]=(uint8_t)op;
    for (int i=0;i<6;i++) a[10+i]=mac[i];
    for (int i=0;i<4;i++) a[16+i]=my_ip[i];
    for (int i=0;i<6;i++) a[20+i]=dst_mac[i];
    for (int i=0;i<4;i++) a[26+i]=tgt_ip[i];
    uint8_t bcast[6]={0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
    eth_tx(op==1?bcast:dst_mac, 0x0806, a, 28);
}

static void icmp_send(const uint8_t *dst_ip, const uint8_t *dst_mac, uint8_t type, uint16_t seq, const uint8_t *data, int dlen){
    uint8_t p[64];
    int total = 20 + 8 + dlen;
    p[0]=0x45; p[1]=0; p[2]=(uint8_t)(total>>8); p[3]=(uint8_t)total;
    p[4]=0; p[5]=1; p[6]=0x40; p[7]=0; p[8]=64; p[9]=1; p[10]=0; p[11]=0;
    for (int i=0;i<4;i++){ p[12+i]=my_ip[i]; p[16+i]=dst_ip[i]; }
    uint16_t cs = cksum(p,20);
    p[10]=(uint8_t)(cs>>8); p[11]=(uint8_t)cs;
    int o=20;
    p[o++]=type; p[o++]=0;
    p[o++]=0; p[o++]=0;
    p[o++]=0x12; p[o++]=0x34;
    p[o++]=(uint8_t)(seq>>8); p[o++]=(uint8_t)seq;
    for (int i=0;i<dlen;i++) p[o++]=data[i];
    uint16_t ic = cksum(p+20, 8+dlen);
    p[22]=(uint8_t)(ic>>8); p[23]=(uint8_t)ic;
    eth_tx(dst_mac, 0x0800, p, total);
}

static void handle_packet(uint8_t *p, int len){
    if (len<14) return;
    uint16_t type=(uint16_t)((p[12]<<8)|p[13]);
    if (type==0x0806 && len>=42){
        uint16_t op=(uint16_t)((p[20]<<8)|p[21]);
        if (op==1 && p[38]==my_ip[0] && p[39]==my_ip[1] && p[40]==my_ip[2] && p[41]==my_ip[3]){
            arp_send(2, p+6, p+28);
        } else if (op==2){
            for (int i=0;i<6;i++) peer_mac[i]=p[22+i];
            for (int i=0;i<4;i++) peer_ip[i]=p[28+i];
            peer_known=1;
        }
        return;
    }
    if (type==0x0800 && len>=34){
        int ihl=(p[14]&0x0F)*4;
        if (p[14+9]!=1) return;
        int io=14+ihl;
        if (len < io+8) return;
        if (p[io]==8){
            icmp_send(p+12, p+6, 0, (uint16_t)((p[io+6]<<8)|p[io+7]), p+io+8, len-io-8);
        } else if (p[io]==0 && p[io+4]==0x12 && p[io+5]==0x34){
            ping_done=1;
        }
    }
}

void net_poll(void){
    uint8_t b[2048];
    int n;
    while ((n=rx_recv(b,2048))) handle_packet(b,n);
}

void net_cmd_status(void){
    if (!net_ok){ kputs("net: no e1000 device\n"); return; }
    net_poll();
    kputs("e1000: MAC ");
    for (int i=0;i<6;i++){ khex2(mac[i]); if(i<5) kputc(':'); }
    kputs("\nIP 10.0.2.15  mask 255.255.255.0  gw 10.0.2.2\n");
    kputs(rd32(0x0000)&0x40 ? "link: up\n" : "link: down\n");
}

int net_cmd_ping(const char *args){
    if (!net_ok){ kputs("ping: no e1000 device\n"); return -1; }
    uint8_t ip[4]; int n=0, acc=0;
    for (int i=0;;i++){
        char c=args[i];
        if (c>='0'&&c<='9'){ acc=acc*10+(c-'0'); continue; }
        if (n>3) break;
        ip[n++]=(uint8_t)acc; acc=0;
        if (c==0 || n==4) break;
    }
    if (n!=4){ kputs("Usage: ping A.B.C.D\n"); return -1; }
    peer_known=0;
    arp_send(1, peer_mac, ip);
    uint32_t t0=timer_get_ticks();
    while (!peer_known && timer_get_ticks()-t0 < 200) net_poll();
    if (!peer_known){ kputs("ping: no ARP reply\n"); return -2; }
    ping_done=0;
    uint16_t seq=++ping_seq;
    icmp_send(ip, peer_mac, 8, seq, (const uint8_t*)"NEXUSOS!", 8);
    t0=timer_get_ticks();
    while (!ping_done && timer_get_ticks()-t0 < 300) net_poll();
    if (!ping_done){ kputs("ping: timeout\n"); return -3; }
    kputs("reply from ");
    knum(ip[0]); kputc('.'); knum(ip[1]); kputc('.'); knum(ip[2]); kputc('.'); knum(ip[3]);
    kputs(" time="); knum((timer_get_ticks()-t0)*10); kputs(" ms\n");
    return 0;
}
