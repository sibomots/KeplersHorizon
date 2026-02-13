# How to Build Kepler's Horizon


This page explains the pre-requisites and setup for building
the software and deploying the software on your target web server.


## Prerequisites

- A C/C++ compiler
- An Apache web server (although any other kind may do, they were not tested)
- cmake
- make
- perl


## Parts to Build

1. There is a MySQL database to create and populate (seed) prior to operation.

2.  There is a server-side component that implements a REST server.  The source
for that is in site/server

3.  There are web content files. These are the static HTML/JavaScript files
that are deployed on your web site.


### Database

- Locate site/db/
- Execute the `schema.sql`
- Execute the `seed.sql`

For example:

```
$ mysql --local-infile=1 -u USER -D mysql -p < schema.sql
$ mysql --local-infile=1 -u USER -D khdb -p < seed.sql
```

The database creation step via `schema.sql` is **destructive.**  It will 
drop the entire database and re-create it.

A tip:  Create a *MySQL user* `USER` that has all the privileges of `root` 
so that you won't have to become `root` in the shell prior to execution.

Note that the `-D mysql` is necessary when the Database is created.

Note that the `-D khdb` is necessary when the Database is **seeded.**

If everything was created correctly, there will be no errors on the
command line.

### Building the  REST server

Let's call this simply 'server' not to be confused with the Apache web server.
When we use the term **server** we are referring to the REST-ful server that
is compiled from this package.

This is a stand-alone application in C++ that runs on the target.  It
handles all of the game logic and dialog between the client (web browser)
and the back-end.

Your Apache configuration will need to be adjusted for the (virtual) server
you choose.

This sort of stanza needs to be added to the virtual server configuration:

```
   # --- proxy API only ---
   ProxyPreserveHost On
   ProxyPass        /kh/api/  http://127.0.0.1:8080/api/
   ProxyPassReverse /kh/api/  http://127.0.0.1:8080/api/
```

Build the server:

```
$ cmake -B build -S .
$ make -C build
```


Setup running the server.  A useful thing is to make a shell script that
passes the arguments to run the server:

```
build/kh --dbuser SOME_USER --dbpass SOME_PASSWORD --dbname khdb --port 8080
```

Then simply doing:

```
$ bash run.sh
```

Will kick off the server.


If the server is running, let it run and push that window aside.

Open a new Window


### The Web Content

The web content is static HTML and JavaScript that interacts with the
proxy and the Apache web server.

Copy the contents of `site/web/*` to where you host the game.  Keep in mind
the paths you selected in the Proxy configuration.  The default expected is:

```
/kh
```

So, if your domain is `example.com`, then make a directory in your `DocumentRoot` called `kh` and place all of the files from `site/web/*` into `DocumentRoot/kh/`

If the files for the `DocumentRoot` of `example.com` are located in the
filesystem as `/var/www/example`.

In other words if the home page of `example.com` is at `/var/www/example/index.html` then this is the layout:

```
/var/www/example/kh
├── behavior.js
├── constraints.js
├── index.html
├── interface.js
├── map_view.html
├── map_view.template.html
└── slate.js
```

# Testing the Installation

Two users are created for testing.

- Alice (username `alice`, password `alicepw`)
- Bob (username `bob`, password `bobpw`)

Open two web browser windows.

- In one of them, load the URL (http://example.com/kh)
- Login as Alice

- In the other window, load the URL (http://example.com/kh)
- Login as Bob





