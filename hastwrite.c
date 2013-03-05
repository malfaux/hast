#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include "macros.h"
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#define _GNU_SOURCE
#include "hast.h"
#include <getopt.h>
extern unsigned int hashit(char *key, int keylen, uint32_t hashinit);



struct hashbucket {
	int count;
	struct hashslot *slots;
};

struct hashtable_t hashtable = {
		.count = 0,
		.slots = NULL
};
/*
 * TODO: bind dist_table to hashtable
 */
int *dist_table = NULL;

char *lookup(char *key, int len)
{
	int slotindex;
	uint32_t hashval;
	hashval = hashit(key, len, 0);
	int d = dist_table[hashval % hashtable.count];

	if (d < 0) {
		return hashtable.slots[-d-1].data;
	}
	hashval = hashit(key, len, d);
	slotindex = hashval % hashtable.count;
	//printf("lookup[dist_val] = %d, slotindex=%4d hashval=%u [%40s]\n", d, slotindex, hashval, key);
	return hashtable.slots[slotindex].data;
}

static int bucketsort(const void *x, const void *y)
/* reverse sorting of bucket list against bucket count */
{
	struct hashbucket *a = (struct hashbucket *)x;
	struct hashbucket *b = (struct hashbucket *)y;
	return (a->count < b->count)?1:(a->count > b->count)?-1:0;
}

static int bucketprint(struct hashbucket *a, int size)
{
	int i = 0;
	int zerocounts = 0;
	int poolcounts = 0;
	int onecounts = 0;
	int maxpool = 0;
	for (i = 0; i < size; i++) {
		if (a[i].count == 0) {
			zerocounts++;
			continue;
		}
		if (a[i].count == 1) {
			onecounts++;
			continue;
		}
		if (maxpool < a[i].count) maxpool = a[i].count;
		//printf("bucket[%d].count=%d\n", i, a[i].count);
		poolcounts++;
	}
    //if (maxpool == 1) maxpool = 0;
#if 0
	fprintf(stderr, "%d buckets, %d zero, %d ones, %d pooled, maxpool=%d\n",
			size, zerocounts, onecounts, poolcounts, maxpool);
#endif
	return maxpool;
}

static int slotbusy(int *slots, int len, int slot)
{
	while (--len >= 0) {
		if (slots[len] == slot) return 1;
	}
	return 0;
}

int copyslot(struct hashslot *to, struct hashslot *from)
{
	to->data = from->data;
	to->hash = from->hash;
	to->key  = from->key;
	to->keylen = from->keylen;
    to->datalen = from->datalen;
	return ++hashtable.count;
}

int nextfreeslot(int *next, int size)
{
	int pos = *next;
	int found = -1;
	while (pos < size) {
		if (hashtable.slots[pos].key == NULL) {
			found = pos;
			*next = ++pos;
			break;
		}
		pos++;
	}
	return found;
}

void hast_print(void)
{
	int i;
	for (i = 0; i<hashtable.count; i++) {
		fprintf(stderr, "[%4d] -> [%10u,%2lu:%40s] [%d]\n", i, hashtable.slots[i].hash,
				hashtable.slots[i].keylen, hashtable.slots[i].key, dist_table[i]);
	}
}
/* TODO: ASAP! assure uniqueness of the keys!!! */
//int hastmake(struct hashslot *hslots, int keycount)


static struct hashslot *kvbufs = NULL;
static int kvbufsz = 0;
static int kvbufi = -1;
#ifndef KVBUFS_DEFSZ 
#define KVBUFS_DEFSZ 512
#endif
#ifndef KVBUFS_PLUS
#define KVBUFS_PLUS 128
#endif



void hast_write(void)
{
    int i;
    int offset = 0;
    int v = VERSION(CURRENT_VERSION);
    int pagesize = getpagesize();
    int headerlen;
    int padlen;
    char *padbytes;
    write(OUTFD, "HAST", 4);
    write(OUTFD, &v, sizeof(unsigned int));
    write(OUTFD, &hashtable.count, sizeof(int));
    write(OUTFD, dist_table, hashtable.count * sizeof(int));
    
    
    for (i = 0; i < hashtable.count; i++) {
        write(OUTFD, &offset, sizeof(int));
        write(OUTFD, &(hashtable.slots[i].datalen), sizeof(int));
        write(OUTFD, &(hashtable.slots[i].keylen), sizeof(int));
        //printf("OFFSET: %d, [%d,%d]\n", offset, hashtable.slots[i].datalen, hashtable.slots[i].keylen);
        offset = offset + hashtable.slots[i].datalen + hashtable.slots[i].keylen;
    }
    headerlen = 4 + 2*sizeof(int) + hashtable.count * sizeof(int) + 3 * sizeof(int) * hashtable.count + sizeof(int);
    //headerlen = 4 + sizeof(int) * (3 + 4*hashtable.count );
    padlen = pagesize - headerlen % pagesize;
#if 0
    fprintf(stderr, "PADLEN=%d headerlen=%d\n", padlen, headerlen);
#endif
    padbytes = (char *)malloc(padlen);
    memset(padbytes, 0xff, padlen);
    write(OUTFD, &padlen, sizeof(int));
    write(OUTFD, padbytes, padlen);
    for (i = 0; i < hashtable.count; i++) {
        write(OUTFD, hashtable.slots[i].data, hashtable.slots[i].datalen);
        write(OUTFD, hashtable.slots[i].key, hashtable.slots[i].keylen);
    }
}

int hast_make(void)
{
	int k,j,i;
	uint32_t hashval;
	int maxpool;
	int dist;
	int *slots = NULL;
	uint32_t slotindex = 0;
	int curslot = 0;
	int nextslot;
	int freeslot;
    
    int keycount = kvbufi + 1  ;
    //printf("keycount=%d, kvbufi=%d\n", keycount, kvbufi);

	struct hashbucket *buckets = (struct hashbucket *)malloc(sizeof(struct hashbucket) * keycount);
	ERR_IF(buckets == NULL, "malloc(buckets)");
	memset(buckets, 0, sizeof(struct hashbucket) * keycount);
	dist_table = (int *)malloc(sizeof(int)*keycount);
	ERR_IF(dist_table == NULL, "malloc(distable):" );
	memset(dist_table, 0, sizeof(int) * keycount);
	hashtable.slots = (struct hashslot *)malloc(sizeof(struct hashslot) * keycount);
	memset(hashtable.slots, 0, sizeof(struct hashslot) * keycount);
	hashtable.count = 0;
	ERR_IF (hashtable.slots == NULL, "alloc(slots):");



	for (i = 0; i < keycount; i++) {
		kvbufs[i].hash = hashit(kvbufs[i].key, kvbufs[i].keylen, 0);
		hashval = kvbufs[i].hash % keycount;
		buckets[hashval].slots = (struct hashslot *)realloc(
				buckets[hashval].slots,
				sizeof (struct hashslot) * (buckets[hashval].count + 1)
		);
		ERR_IF (buckets[hashval].slots == NULL, "realloc(bucket.keys): ");
		//copyslot(&buckets[hashval].slots[buckets[hashval].count++],&kvbufs[i]);
		//buckets[hashval].slots[buckets[hashval].count++] = kvbufs[i];
		buckets[hashval].slots[buckets[hashval].count].data = kvbufs[i].data;
		buckets[hashval].slots[buckets[hashval].count].keylen = kvbufs[i].keylen;
		buckets[hashval].slots[buckets[hashval].count].datalen= kvbufs[i].datalen;
		buckets[hashval].slots[buckets[hashval].count].key = kvbufs[i].key;
		buckets[hashval].slots[buckets[hashval].count].hash = kvbufs[i].hash;
		buckets[hashval].count++;
	}

	/* TODO: use heapsort for big hashbuckets? */
	qsort(buckets, keycount, sizeof(struct hashbucket), bucketsort);
	maxpool = bucketprint(buckets, keycount);
	slots = (int *)malloc(sizeof(int) * maxpool);
	for (i = 0; /*i < keycount &&*/ buckets[i].count > 1; i++) {
#if 0
        printf("process bucket %d with count %d\n", i, buckets[i].count);
#endif
        //ERROR? slots array was never zeroed!!
        memset(slots, 0xff, sizeof(int) * maxpool);
		dist = 1;
		slotindex = 0;
		curslot = 0;
		j = 0;
		while (j < buckets[i].count) {
			slotindex = hashit(buckets[i].slots[j].key, buckets[i].slots[j].keylen, dist);
#if 0
            unsigned int f;
            f = slotindex;
#endif
			slotindex = slotindex % keycount;
			if (slotbusy(slots, curslot+1, slotindex) || (hashtable.slots[slotindex].key != NULL)) {
				dist++;
#if 0
                printf("SLOT BUSY: slotindex=%d, key=%p, curslot=%d, dist=%d, key=\"%s\", hash=%u\n", 
                        slotindex, hashtable.slots[slotindex].key,curslot, dist, buckets[i].slots[j].key, f);
#endif
				j = 0;
				curslot = 0;
			} else {
#if 0
                printf("SLOT FREE: slotindex=%d, key=%p, curslot=%d, dist=%d, key=\"%s\", hash=%u\n", 
                        slotindex, hashtable.slots[slotindex].key,curslot, dist, buckets[i].slots[j].key, f);
#endif
				slots[curslot++] = slotindex;
				j++;
			}
		}
		dist_table[buckets[i].slots[0].hash % keycount] = dist;
		for (k = 0; k < buckets[i].count; k++) {
			copyslot(&hashtable.slots[slots[k]], &buckets[i].slots[k]);
		}

	}
#if 0
    printf("DONE\n");
#endif
	free(slots); slots = NULL;
	for (j = i, nextslot = 0; j < keycount && buckets[j].count > 0; j++) {
		ERR_IF ( (freeslot = nextfreeslot(&nextslot, keycount)) < 0, "nextfreeslot():");
		dist_table[buckets[j].slots[0].hash % keycount] = -freeslot-1;
		copyslot(&hashtable.slots[freeslot], &buckets[j].slots[0]);
	}
	for (i = 0; i < keycount; i++) {
		free(buckets[i].slots); buckets[i].slots = NULL;
	}
	free(buckets); buckets = NULL;

	//kvbufs remain referenced in hashtable don't free'em

	return 0;
}

static unsigned int scan_uint(register char *s,register unsigned int *u)
{
    register unsigned int pos = 0;
    register unsigned int result = 0;
    register unsigned int c;
    while ((c = (unsigned int) (unsigned char) (s[pos] - '0')) < 10) {
        result = result * 10 + c;
        ++pos;
    }
    *u = result;
    return pos;
}

/*
 *  +3,5:one->Hello
 *  +3,7:two->Goodbye
 */
#define NODATA(buf) errx(1, "malformed data (feedcount %d, line %d): \"%s\"", i, __LINE__, buf) 



static void kvbufs_init(void)
{
    kvbufs = (struct hashslot *)malloc(sizeof(struct hashslot) * KVBUFS_DEFSZ);
    if (!kvbufs) err(1, "malloc(slots): ");
    kvbufsz = KVBUFS_DEFSZ;
}

static void kvbufs_set(void)
{
    if (++kvbufi == kvbufsz) {
        kvbufs = (struct hashslot *)realloc(kvbufs, sizeof(struct hashslot) * (kvbufsz + KVBUFS_PLUS));
        if (!kvbufs) err(1, "realloc(slots): ");
        kvbufsz += KVBUFS_PLUS;
    }
}


static void scan_kvs(int fd)
{
    unsigned int scanpos;
    int pos_;
    unsigned int keylen;
    unsigned int datalen;
    char *key = NULL;
    char *data = NULL;
    int bytesread = 0;
    char *buf = NULL;
    int len = 0;
    int i = 0;
    char kvdelim[2] = "->";
    char eol = '\n';
    FILE *fp;
    ERR_IF( (fp = fdopen(fd, "r")) == NULL, "fdopen(): " );

    kvbufs_init();

    while(1) {
        bytesread = getdelim(&buf, (size_t *)&len, ':', fp);
        if (bytesread == -1) {
            if (feof(fp)) break;
            else NODATA(buf);
        }
        //TODO:  add comment support ? (lines starting with #)
        //printf("BYTESREAD = %d\n", bytesread);
        if (bytesread < 5) NODATA(buf);
        if (buf[bytesread-1] != ':') NODATA(buf);
        if (buf[0] != '+') NODATA(buf);

        scanpos = scan_uint(buf + 1, &keylen);
        if (!scanpos) NODATA(buf);
        if (buf[++scanpos] != ',') NODATA(buf);
        if (keylen == 0) NODATA(buf);

        //printf("scan datalen at %s\n", buf + scanpos + 1);
        pos_ = scan_uint(buf + (++scanpos), &datalen);
        if (pos_ == 0) NODATA(buf);
        scanpos += pos_;
        //printf("bytesread=%d, scanpos=%d\n", bytesread, scanpos);
        //printf("keylen=%d, datalen=%d, buf[scanpos]=%c\n", keylen, datalen, buf[scanpos]);
        if ( bytesread != ++scanpos) NODATA(buf);
        kvbufs_set();

        if (! (kvbufs[kvbufi].key = malloc(keylen+1)) ) NODATA("malloc(key)");
        bytesread = fread(kvbufs[kvbufi].key, 1, keylen, fp);
        if (bytesread != keylen) NODATA(key);
        kvbufs[kvbufi].keylen = keylen;
        kvbufs[kvbufi].key[keylen] = '\0';

        bytesread = fread(kvdelim, 1, 2, fp);
        if (bytesread != 2) NODATA(kvdelim);
        if (kvdelim[0] != '-' || kvdelim[1] != '>') NODATA(kvdelim);


        if (! (kvbufs[kvbufi].data= malloc(datalen+1)) ) NODATA("malloc(key)");
        bytesread = fread(kvbufs[kvbufi].data, 1, datalen, fp);
        if (bytesread != datalen) NODATA(data);
        kvbufs[kvbufi].datalen = datalen;
        kvbufs[kvbufi].data[datalen] = '\0';
        //printf("SCAN: keylen=%d, datalen=%d, key=%s, data=%s, kvi=%d\n", keylen, datalen, kvbufs[kvbufi].key, kvbufs[kvbufi].data, kvbufi);
        // read \n
        bytesread = fread(&eol, 1, 1, fp);
        if (bytesread == 0) { //handle short writes
            if (feof(fp) ) break;
            err(1, "shortread");
        }
        if (eol != '\n') err(1, "malformed data");
        i++;
    }
    //printf("%d parsed records\n", kvbufi);
    free(buf);
    fclose(fp);
    //return hastmake();
    return;
}

void hast_feed(void) { return scan_kvs(0); }

//#define hast_feed() scan_kvs(0)


