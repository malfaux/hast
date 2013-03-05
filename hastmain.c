#include <unistd.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hast.h"

extern void hast_feed(void);
extern int hast_make(void);
extern void hast_write(void);
extern void hast_init(char *);
extern int hast_find(char *key, int keylen, char **data, int *datalen);
extern void hast_close(void);


extern char *optarg;
int query_mode = 0;
int write_mode = 0;
const char *p;

static void usage(void)
{
    fprintf(stderr, "usage: %s -q -f <hastdatabase>\n\
\t-q: query mode. queries the database file specified with -f. input keys are read from stdin\n\
usage: %s -w\n\
\tcompiles data from standard input into hast format and writes it to standard output\n\
\tinput format: +keylen,datalen:key->data\\n\n", p, p);
    exit(1);
}

int main(int argc, char **argv)
{
    int rc;
    int query_mode = 0;
    int write_mode = 0;
    char *dbfn = NULL;
    char *data = NULL;
    int datalen = 0;
    size_t buflen = 0;
    char *buf = NULL;
    int n;
    int opt;
    p = argv[0];

    while ( (opt = getopt(argc, argv, "wqf:")) != -1) {
        switch (opt) {
            case 'w':
                write_mode = 1;
                break;
            case 'q':
                query_mode = 1;
                break;
            case 'f':
                n = strlen(optarg);
                dbfn = (char *)malloc(n + 1);
                memcpy(dbfn, optarg, n);
                dbfn[n] = '\0';
                break;
            default:
                fprintf(stderr, "unknown option %c\n" , opt);
                usage();
                break;
        }
    }

    if ( (query_mode & write_mode) || !(query_mode | write_mode ) ) {
        fprintf(stderr, "bad flags\n");
        usage();
    }

    if (write_mode) {
        hast_feed();
        hast_make();
        hast_write();
        return 0;
    }
    if (!dbfn) {
        fprintf(stderr, "no database\n");
        usage();
    }
    hast_init(dbfn);
    while (1) {
        rc = getline(&buf, &buflen, stdin);
        if (rc == -1) {
            if (feof(stdin)) break;
            err(1,"getline(): ");
        }
        buf[rc-1] = '\0';
        hast_find(buf, rc-1, &data, &datalen);
        printf("hdb[%s] = \"%s\"\n", buf, data);
    }
    hast_close();
    return 0;
}

