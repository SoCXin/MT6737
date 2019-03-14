#include "kdf.h"
#include "sha.h"
#define NEUSOFT_ID "neusoft"
#define ID_LEN  6
#define KEY_LEN 32



unsigned int kdflib_get_msg_auth_key(unsigned char *hwid, unsigned int
huk_size, unsigned char *key, unsigned int key_size)
{
     unsigned char huk_key[]={110,101,117,115,111,102,100};
     if(key_size != KEY_LEN)
     {
         return -1;
     }
     byte * tmp;
     SHA256_CONTEXT hd ;
     int counter = 0;
     sha256_init1(&hd);

     sha256_write(&hd, hwid, huk_size);
     sha256_write(&hd, (byte*)&counter, sizeof(int));
     sha256_write(&hd, huk_key, ID_LEN);

      sha256_final(&hd);
      tmp = sha256_read(&hd);
     memcpy(key,tmp,key_size);
     return 0;
}
unsigned int kdflib_get_huk(unsigned char * key,unsigned int 
key_size,unsigned char *id, unsigned int
id_size, unsigned char *huk, unsigned int huk_size)
{
     unsigned char huk_key[]={110,101,117,115,111,102,100};
     if(huk_size != KEY_LEN)
     {
         return -1;
     }
     byte * tmp;
     SHA256_CONTEXT hd ;
     int counter = 0;
     sha256_init1(&hd);

     sha256_write(&hd, key, key_size);
     sha256_write(&hd, (byte *)&counter, sizeof(int));
     sha256_write(&hd, huk_key, ID_LEN);
     sha256_write(&hd, id, id_size);

      sha256_final(&hd);
      tmp = sha256_read(&hd);
     memcpy(huk,tmp,huk_size);
     return 0;

}
