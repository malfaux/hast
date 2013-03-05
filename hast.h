#ifndef HAST_H_
#define HAST_H_
#include <unistd.h>
#include <stdint.h>
struct hashslot {
	char *key;
	size_t keylen;
	char *data;
    size_t datalen;
	uint32_t hash;
};
typedef struct hashslot hashslot_t;
struct hashtable_t {
	int count;
	struct hashslot *slots;
};

/* versioning scheme: A.B.C-D
 * A - software version. an increment means changes in the concepts/philosophy/architecture
 *   - denotes changes which are more philosophical in their nature and it's possible not to bump/affect B
 * B - major revision - denotes changes in the api or data formats which will raise incompatibilities between releases
 * C - minor revision - new features were added or enhancements were made pertaining to stability and/or speed
 * D - {security,bug}fixes release
 */

#define CURRENT_VERSION "0.0.1-0"                                                                                                              

#define VERSION(v) ( (unsigned int)(unsigned char)(v[0]-'0') << 24) + \
    ( (unsigned int)(unsigned char)(v[2] - '0') << 16) + \
    ( (unsigned int)(unsigned char)(v[4] - '0') << 8) + \
    (unsigned int)(unsigned char)(v[6] - '0')

#define VERSIONX(a,b,c,d) (a << 24) + (b << 16) + (c << 8) + d





#define OUTFD 1
#endif /* HAST_H_ */
