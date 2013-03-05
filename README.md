hast
====

a minimal, perfectly hashed database

build
-----
type: 'make again'

targets: hast, libhast.a, libhast.so

usage
-----

build a database: 

**./hast -w >hast.db <datainput**

the format of datainput is the same as for a cdb writer:

*+keylen,datalen:key->data\\n*

query a database: **./hast -q -f ./hast.db <querykeys.txt**

API access (link against libhast)
---------------------
* void hast_open(char *database_file_name);
* int hast_find(char *key, int keylen, char **data, int *datalen);
* void hast_close(void);

NOTES:

* if any errors are encountered while trying to open the database (hast_open), the program will die
* hast_find returns -1 if the key was not found in the database.

