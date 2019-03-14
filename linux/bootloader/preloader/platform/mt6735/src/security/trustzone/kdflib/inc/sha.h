#ifndef __SHA_H__
#define __SHA_H__

typedef unsigned int u32;
typedef unsigned char byte;

typedef struct {
    u32  h0,h1,h2,h3,h4,h5,h6,h7;
    u32  nblocks;
    byte buf[64];
    int  count;
} SHA256_CONTEXT;


#define DIM(v) (sizeof(v)/sizeof((v)[0]))
#define wipememory2(_ptr,_set,_len) do { volatile char *_vptr=(volatile char *)(_ptr); unsigned int _vlen=(_len); while(_vlen) { *_vptr=(_set); _vptr++; _vlen--; } } while(0)
#define wipememory(_ptr,_len) wipememory2(_ptr,0,_len)
#define __memset__(s,c,n) {byte *p = (byte*)(s); while((n)-- > 0){*p++=(c);}}
#define __memcpy__(d,s,n) {byte *p=(byte*)(d); byte *q=(byte*)(s); while((n)-->0){*p++=*q++;}}
#define rol(x, n) ( ((x)<<(n)) | ((x)>>(32-(n))) )

void sha256_init1( SHA256_CONTEXT *hd );
void sha256_write( SHA256_CONTEXT *hd, byte *inbuf, unsigned int inlen);
void sha256_final(SHA256_CONTEXT *hd);
byte *sha256_read( SHA256_CONTEXT *hd );

#endif