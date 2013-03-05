hast
====

a minimal, perfectly hashed database

build
-----
type: 'make again'
targets: hast, libhast.a, libhast.so

usage
-----

build a database: ./hast -w <datainput >hast.db
query a database: echo key | ./hast -q -f ./hast.db

