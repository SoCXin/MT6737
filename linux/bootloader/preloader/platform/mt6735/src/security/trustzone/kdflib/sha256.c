#include <string.h>
#include "inc/sha.h"
static void
burn_stack (int bytes)
{
    char buf[128];

    wipememory(buf,sizeof buf);
    bytes -= sizeof buf;
    if (bytes > 0)
        burn_stack (bytes);
}


void
sha256_init1( SHA256_CONTEXT *hd )
{
    hd->h0 = 0x6a09e667;
    hd->h1 = 0xbb67ae85;
    hd->h2 = 0x3c6ef372;
    hd->h3 = 0xa54ff53a;
    hd->h4 = 0x510e527f;
    hd->h5 = 0x9b05688c;
    hd->h6 = 0x1f83d9ab;
    hd->h7 = 0x5be0cd19;

    hd->nblocks = 0;
    hd->count = 0;
}


/****************
 * Transform the message w which consists of 16 32-bit words
 */
static void
transform( SHA256_CONTEXT *hd, byte *data )
{
  u32 a,b,c,d,e,f,g,h;
  u32 w[64];
  int t;
  static const u32 k[]=
    {
      0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1,
      0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
      0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786,
      0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
      0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147,
      0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
      0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b,
      0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
      0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a,
      0x5b9cca4f, 0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
      0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
    };

  /* get values from the chaining vars */
  a = hd->h0;
  b = hd->h1;
  c = hd->h2;
  d = hd->h3;
  e = hd->h4;
  f = hd->h5;
  g = hd->h6;
  h = hd->h7;

#ifdef BIG_ENDIAN_HOST
  memcpy( w, data, 64 );
#else
  {
    int i;
    byte *p2;

    for(i=0, p2=(byte*)w; i < 16; i++, p2 += 4 )
      {
	p2[3] = *data++;
	p2[2] = *data++;
	p2[1] = *data++;
	p2[0] = *data++;
      }
  }
#endif

#define ROTR(x,n) (((x)>>(n)) | ((x)<<(32-(n))))
#define Ch(x,y,z) (((x) & (y)) ^ ((~(x)) & (z)))
#define Maj(x,y,z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define Sum0(x) (ROTR((x),2) ^ ROTR((x),13) ^ ROTR((x),22))
#define Sum1(x) (ROTR((x),6) ^ ROTR((x),11) ^ ROTR((x),25))
#define S0(x) (ROTR((x),7) ^ ROTR((x),18) ^ ((x)>>3))
#define S1(x) (ROTR((x),17) ^ ROTR((x),19) ^ ((x)>>10))

  for(t=16;t<64;t++)
    w[t] = S1(w[t-2]) + w[t-7] + S0(w[t-15]) + w[t-16];

  for(t=0;t<64;t++)
    {
      u32 t1,t2;

      t1=h+Sum1(e)+Ch(e,f,g)+k[t]+w[t];
      t2=Sum0(a)+Maj(a,b,c);
      h=g;
      g=f;
      f=e;
      e=d+t1;
      d=c;
      c=b;
      b=a;
      a=t1+t2;
      /* printf("t=%d a=%08lX b=%08lX c=%08lX d=%08lX e=%08lX f=%08lX g=%08lX h=%08lX\n",t,a,b,c,d,e,f,g,h); */
    }

  /* update chaining vars */
  hd->h0 += a;
  hd->h1 += b;
  hd->h2 += c;
  hd->h3 += d;
  hd->h4 += e;
  hd->h5 += f;
  hd->h6 += g;
  hd->h7 += h;
}


/* Update the message digest with the contents
 * of INBUF with length INLEN.
 */
void
sha256_write( SHA256_CONTEXT *hd, byte *inbuf, unsigned int inlen)
{
    if( hd->count == 64 ) { /* flush the buffer */
	transform( hd, hd->buf );
        burn_stack (328);
	hd->count = 0;
	hd->nblocks++;
    }
    if( !inbuf )
	return;
    if( hd->count ) {
	for( ; inlen && hd->count < 64; inlen-- )
	    hd->buf[hd->count++] = *inbuf++;
	sha256_write( hd, 0, 0 );
	if( !inlen )
	    return;
    }

    while( inlen >= 64 ) {
	transform( hd, inbuf );
	hd->count = 0;
	hd->nblocks++;
	inlen -= 64;
	inbuf += 64;
    }
    burn_stack (328);
    for( ; inlen && hd->count < 64; inlen-- )
	hd->buf[hd->count++] = *inbuf++;
}


/* The routine final terminates the computation and
 * returns the digest.
 * The handle is prepared for a new cycle, but adding bytes to the
 * handle will the destroy the returned buffer.
 * Returns: 32 bytes representing the digest.
 */

void
sha256_final(SHA256_CONTEXT *hd)
{
    u32 t, msb, lsb;
    byte *p;

    sha256_write(hd, 0, 0); /* flush */;

    t = hd->nblocks;
    /* multiply by 64 to make a byte count */
    lsb = t << 6;
    msb = t >> 26;
    /* add the count */
    t = lsb;
    if( (lsb += hd->count) < t )
	msb++;
    /* multiply by 8 to make a bit count */
    t = lsb;
    lsb <<= 3;
    msb <<= 3;
    msb |= t >> 29;

    if( hd->count < 56 ) { /* enough room */
	hd->buf[hd->count++] = 0x80; /* pad */
	while( hd->count < 56 )
	    hd->buf[hd->count++] = 0;  /* pad */
    }
    else { /* need one extra block */
	hd->buf[hd->count++] = 0x80; /* pad character */
	while( hd->count < 64 )
	    hd->buf[hd->count++] = 0;
	sha256_write(hd, 0, 0);  /* flush */;
	memset(hd->buf, 0, 56 ); /* fill next block with zeroes */
    }
    /* append the 64 bit count */
    hd->buf[56] = msb >> 24;
    hd->buf[57] = msb >> 16;
    hd->buf[58] = msb >>  8;
    hd->buf[59] = msb	   ;
    hd->buf[60] = lsb >> 24;
    hd->buf[61] = lsb >> 16;
    hd->buf[62] = lsb >>  8;
    hd->buf[63] = lsb	   ;
    transform( hd, hd->buf );
    burn_stack (328);

    p = hd->buf;
#ifdef BIG_ENDIAN_HOST
#define X(a) do { *(u32*)p = hd->h##a ; p += 4; } while(0)
#else /* little endian */
#define X(a) do { *p++ = hd->h##a >> 24; *p++ = hd->h##a >> 16;	 \
		      *p++ = hd->h##a >> 8; *p++ = hd->h##a; } while(0)
#endif
    X(0);
    X(1);
    X(2);
    X(3);
    X(4);
    X(5);
    X(6);
    X(7);
#undef X
}

byte *
sha256_read( SHA256_CONTEXT *hd )
{
    return hd->buf;
}

