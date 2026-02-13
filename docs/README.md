# Documentation

The documentation is here.  It will be changed as new features are finished.


## Man Page

The good ol' manpage:

```
$ man -l kh.1
```


## PDF

You'll need `groff` and `ghostscript`

To build the manpage as PDF:

```
$ groff -t -man -Tpdf kh.1 > kh.pdf
```


