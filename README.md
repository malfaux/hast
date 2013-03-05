hast
====

a minimal, perfectly hashed database

build
-----
type: 'make again'

targets: hast, libhast.a, libhast.so

usage
-----

build a database: ./hast -w &lt;datainput &gt;hast.db

the format of datainput is the same as for a cdb writer:

*+keylen,datalen:key->data\\n*

query a database: echo key | ./hast -q -f ./hast.db

API access (for programs linked against libhast)
---------------------
* void hast_init(char *database_file_name);
* int hast_find(char *key, int keylen, char **data, int *datalen);
* void hast_close(void);

