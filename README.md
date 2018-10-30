# spamidea

Half-baked tool to make pretty plots of email flow. Currently can only
process Maildir style mailboxes. Emits some interesting output to stdout,
plus creates a 'graphviz' file called `plot2.dot`. 

To compile, run `make`. Requires a very recent C++ compiler. Code depends on
boost (libboost-dev).

Run like this:

```
$ ./getsender ~/Maildir/cur/* > output.txt
$ dot -Tsvg plot2.dot > plot2.svg
```

## What it does
We believe the 'Received' lines in email, and use them to build a tree. We
assume every hop lies about everything beyond, which means that every
'smtp.example.com' is different, unless it shares a hop immediately below
it. This makes the flow a directed acyclic graph. 

We omit hops called 'localhost' & compact hops to the same server name.

