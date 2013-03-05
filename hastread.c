#include <hast.h>
#include <err.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>



//extern char *lookup(char *key, int len);


struct __attribute__((__packed__)) hdb_offsets {
    int offset;
    int datalen;
    int keylen;
};

struct hastdb {
    int fd;
    int rawlen;
    unsigned int version;
    int count;
    int *dist_table;
    struct hdb_offsets *offsets;
    int padlen;
    char *data;
};

struct hastdb hdb;
#define DATA_OFFSET (\
        4 /*DBTYPE*/ + \
        sizeof(int) /*version*/ + \
        sizeof(int) /*count*/ + \
        hdb.count * sizeof(int) /*dist table*/ + \
        hdb.count * sizeof(struct hdb_offsets) /*offsets + k/v len*/ + \
        sizeof(int) /*padlen*/ + \
        hdb.padlen /*padbytes*/ \
        )
//2*sizeof(int) + hdb.count*sizeof(int) + 3 * hdb.count * sizeof(int) + sizeof(int)) 

void hast_open(char *f)
{
    char dbname[5];
    char *padbytes;
    struct stat stbuf;
    memset(&hdb, 0, sizeof(struct hastdb));
    hdb.fd = open(f, O_RDONLY);
    fstat(hdb.fd, &stbuf);
    hdb.rawlen = stbuf.st_size;
    if (hdb.fd == -1) err(1,"can't open %s", f);
    read(hdb.fd, dbname, 4);
    if (strncmp(dbname, "HAST", 4) != 0) errx(1,"nohast");
    dbname[4] = '\0';
    read(hdb.fd, &hdb.version, sizeof(int));
    if (hdb.version != VERSION(CURRENT_VERSION) ) 
        errx(1,"database version mismatch: db:%u, sw:%u", hdb.version, VERSION(CURRENT_VERSION));
    read(hdb.fd, &(hdb.count), sizeof(int));
#if 0
    printf("dbtype=%s, version=%u, num_records=%d\n", dbname, hdb.version, hdb.count);
#endif
    hdb.dist_table = (int *)malloc(sizeof(int) * hdb.count);
    read(hdb.fd, hdb.dist_table, sizeof(int) * hdb.count);

    hdb.offsets = (struct hdb_offsets *)malloc(sizeof(struct hdb_offsets) * hdb.count);
    read(hdb.fd, hdb.offsets, sizeof(struct hdb_offsets) * hdb.count);

    read(hdb.fd, &hdb.padlen, sizeof(int));
    /* also read the pad even if only for brevity */
    padbytes = (char *)malloc(hdb.padlen);
    read(hdb.fd, padbytes, hdb.padlen); 
#if 0
    printf("PADLEN=%d, headerlen=%lu, nopad_headerlen=%lu\n", hdb.padlen, DATA_OFFSET, DATA_OFFSET - hdb.padlen );
#endif
    hdb.data = mmap(NULL, hdb.rawlen - DATA_OFFSET, PROT_READ, MAP_PRIVATE, hdb.fd, DATA_OFFSET);
    if (hdb.data == MAP_FAILED) err(1,"mmap failed: ");
#if 0
    printf("DATA: mapped at %p dbsize=%d, headersize=%lu, datasize=%lu\n",
            hdb.data, hdb.rawlen, DATA_OFFSET, hdb.rawlen - DATA_OFFSET);
#endif
}

void hast_close(void)
{
    if (munmap(hdb.data, hdb.rawlen - DATA_OFFSET) == -1) err(1,"munmap(): ");
    //printf("munmap() OK\n");
    //free(hdb.data);
    close(hdb.fd);
    free(hdb.dist_table);
    free(hdb.offsets);
    bzero(&hdb, sizeof(struct hastdb));
}

extern unsigned int hashit(char *, unsigned int, unsigned int);

int hast_find(char *key, int keylen, char **data, int *datalen)
{
    int slotindex;
    uint32_t hashval;
    int offset;
    int keylen_;
    int datalen_;
    int d;
    hashval = hashit(key, keylen, 0);
    d = hdb.dist_table[hashval % hdb.count];
    if (d < 0) {
        slotindex = -d-1;
    } else {
        hashval = hashit(key, keylen, d);
        slotindex = hashval % hdb.count;
    }

    offset = hdb.offsets[slotindex].offset;
    datalen_ = hdb.offsets[slotindex].datalen;
    keylen_= hdb.offsets[slotindex].keylen;
    if (keylen_ != keylen) {
        /*
        printf("keylen don't match, have %d, found %d, datalen=%d, slotindex=%d, key=\"%s\", offset=%d, distance=%d\n", 
                keylen, keylen_, datalen_, slotindex, key, offset, d);
        */
        return -1;
    }
    if (strncmp(key, hdb.data + offset+datalen_, keylen) != 0) {
        //printf("key content don't match\n");
        return -1;
    }
    if (*data == NULL) {
        *data = (char *)malloc(datalen_ + 1);
        if (*data == NULL) err(1, "malloc");
    } else {
        *data = (char *)realloc(*data, datalen_ + 1);
    }
    memcpy(*data, hdb.data + offset, datalen_);
    (*data)[datalen_] = '\0';
    *datalen = datalen_;
    return 0;
}

void hast_dump(void)
{
    int i;
    char *key = NULL;
    char *data = NULL;
    /*
    for (i = 0; i < hdb.count; i++) {
        //printf("dist_table[%d] = %d\n", i, hdb.dist_table[i]);
    }
    */
    for (i = 0; i < hdb.count; i++) {
        data = (char *)realloc(data, hdb.offsets[i].datalen + 1);
        memcpy(data, hdb.data + hdb.offsets[i].offset, hdb.offsets[i].datalen);
        data[hdb.offsets[i].datalen] = '\0';
        key = (char *)realloc(key, hdb.offsets[i].keylen + 1);
        key[hdb.offsets[i].keylen] = '\0';
        memcpy(key, hdb.data + hdb.offsets[i].offset + hdb.offsets[i].datalen, hdb.offsets[i].keylen);
        printf("offsets[%d] = [off=%d,dlen=%d,klen=%d],key=\"%s\", data=\"%s\", distance=%d\n", 
                i, hdb.offsets[i].offset, hdb.offsets[i].datalen, hdb.offsets[i].keylen, key,data, hdb.dist_table[i]);
    }
    return;

}

#ifdef HASTREAD_MAIN
int main(int argc, char **argv)
{
    char *data = (char *)malloc(3);
    char *buf = NULL;
    size_t buflen = 0;
    int datalen = 3;
    int rc;
    //int i;
    hast_open(argv[1]);
    while (1) {
        rc = getline(&buf, &buflen, stdin);
        if (rc == -1) {
            if (feof(stdin)) break;
            err(1,"getline(): ");
        }
        buf[rc-1] = '\0';
        hast_find(buf, rc-1, &data, &datalen);
        //printf("hdb[%s] = \"%s\"\n", buf, data);
        write(1, data, datalen);
    }
    hast_close();
    exit(0);
    //hast_dump();

    return 0;
}
#endif
