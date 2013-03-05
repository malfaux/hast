#include <unistd.h>
#include <stdint.h>
#include <stdio.h>

#ifndef UINT32_MAX
#define UINT32_MAX (4294967295U)
#endif
#define NOMATCH_HASH (1219579562U)

#ifdef SFHASH
/* gcc -DSFHASH will make the program use the superfasthash function 
 * designed and implemented by Paul Hsieh
 * copyright: Paul Hsieh http://www.azillionmonkeys.com/qed/hash.html
 */
#if (defined(__GNUC__) && defined(__i386__)) || defined(__WATCOMC__) \
            || defined(_MSC_VER) || defined (__BORLANDC__) || defined (__TURBOC__)
        #define GET16B(d) (*((const uint16_t *) (d)))
#else
        #define GET16B(d) ((((uint32_t)(((const uint8_t *)(d))[1])) << 8) \
                        +(uint32_t)(((const uint8_t *)(d))[0]) )
#endif
uint32_t hashit(char *key, uint32_t keylen, uint32_t seed)
{
    char *keyptr = key;
    uint32_t hkey = (seed == 0)?keylen:seed;
    uint32_t tmp;
    uint32_t rem = keylen & 3;
    if (keylen == 0 || keyptr == NULL) return 0;
    keylen >>= 2;

    for (;keylen > 0; keylen--) {
        hkey += GET16B(keyptr);
        tmp = (GET16B(keyptr+2) << 11) ^ hkey;
        hkey = (hkey << 16) ^ tmp;
        keyptr += 2*sizeof(uint16_t);
        hkey += hkey >> 11;
    }
    switch (rem) {
        case 3:
            hkey += GET16B (keyptr);
            hkey ^= hkey << 16;
            hkey ^= keyptr[sizeof (uint16_t)] << 18;
            hkey += hkey >> 11;
            break;
        case 2:
            hkey += GET16B (keyptr);
            hkey ^= hkey << 11;
            hkey += hkey >> 17;
            break;
        case 1:
            hkey += *keyptr;
            hkey ^= hkey << 10;
            hkey += hkey >> 1;
            break;
        default:
            break;
    }
    hkey ^= hkey << 3;
    hkey += hkey >> 5;
    hkey ^= hkey << 4;
    hkey += hkey >> 17;
    hkey ^= hkey << 25;
    hkey += hkey >> 6;
#ifdef DEBUG
    //printf("hashresult=%u\n", hkey);
#endif

    return hkey;
}
#elif defined FNVHASH
/*
 * TODO: see http://www.isthe.com/chongo/tech/comp/fnv/ , 
 * Retry method: for Changing the FNV hash size - non-powers of 2
 */
#define FNV1_32_INIT ((uint32_t)0x811c9dc5)
#define FNV_32_PRIME ((uint32_t)0x01000193)
uint32_t hashit(char *key, uint32_t keylen, uint32_t seed)
{
    unsigned char *bp = (unsigned char *)key;
    unsigned char *be = bp + keylen;
    uint32_t hkey = (seed == 0)?FNV1_32_INIT:seed;
    while(bp < be) {
#if defined(NO_FNV_GCC_OPTIMIZATION)
        hkey *= FNV_32_PRIME;
#else
        hkey += (hkey<<1) + (hkey<<4) + (hkey<<7) + (hkey<<8) + (hkey<<24);
#endif
        hkey ^= (uint32_t)*bp++;
    }
    return hkey;
}
#elif defined CDBHASH
#define CDB_INIT 5381
uint32_t hashit(char *key, uint32_t keylen, uint32_t seed)
{
    uint32_t hkey;
    char *keyptr = key;
    hkey = (seed == 0)?CDB_INIT:seed;
    while (keylen) {
        hkey += (hkey<<5);
        hkey ^= *keyptr++;
        --keylen;
    }
    return hkey;
}
#else
#error "no hashing function defined at compile time -DSFHASH | -DCDBHASH | -DFNVHASH"
#endif


#ifdef HAVEMAIN
#include <string.h>
int main(int argc, char **argv)
{
    printf("%u\n", hashit(argv[1], strlen(argv[1]), 0));
}
#endif
