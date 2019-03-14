#ifndef KDF_H
#define KDF_H
unsigned int kdflib_get_msg_auth_key(unsigned char *huk, unsigned int 
huk_size, unsigned char *key, unsigned int key_size);
unsigned int kdflib_get_huk(unsigned char * key,unsigned int key_size,unsigned char *id, unsigned int 
id_size, unsigned char *huk, unsigned int huk_size);

#endif

