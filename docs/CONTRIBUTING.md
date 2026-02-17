## Contributing

Kepler’s Horizon is a work in progress.

The game will continue to evolve as core gameplay is stabilized and made
playable over the web. New features, rule dynamics, maps, naming, and other
enhancements are planned and will be introduced over time. There is a long
list of ideas and improvements, and the current focus is intentionally narrow:
to deliver a solid, playable implementation of the base game.

Contributors should keep this in mind. Early contributions are expected to
support the existing direction rather than expand scope prematurely. If you
are interested in contributing, please contact the repository owner before
starting substantial work.

Areas where contributions may be especially helpful include:

- Recommendations and analysis related to game design and rule dynamics
- Artwork and visual assets, as the game transitions from purely textual
  interaction to more visual representation
- Sound clips or audio concepts, should audio elements be introduced
- Other thoughtful contributions that align with the long-term direction of
  the project

Discussion and design feedback are welcome, particularly where they help
clarify or strengthen the core mechanics.

# Developing

Software development for Kelpler's Horizon involves three distinct domains of
experience and production.

1.  C++ - The REST server is written in C++17 and likely will remain on that
    version.
2.  SQL - The back-end database that the software supports is MySQL (for the time
    being) and all of the queries and operations for the database are  embedded
    SQL statements invoked by the REST server.  There are a few *do-once* operations
    involved for setting up the database before the server runs.  
3.  JavaScript/CSS/HTML - The front-end user-facing software is primarily HTML/CSS
    tailored content with JavaScript used to conduct REST transactions with
    the server.  The goal is to minimize to practically zero any dynamic behavior
    within JavaScript for the UI.  Instead, REST is used by the server and client
    to wash the UI handling JavaScript with updated messages to alter the content
    of UI elements.
4.  Lisp is used for the AI-Agency

## Other Core Technologies

`lex` and `yacc` (or their GNU equivalents) were the basis for the user-command
processing engine.

This proved to be a far better way to construct the *grammar* of the command
language and augment the command-suite with new commands, handle command
invocation, and so on.

## Design Patterns

There are two main design patterns used.  

1. Singleton
2. Builder

In the server there the phrase *there can be only one* is a phrase that means
in effect we want to isolate and unify behaviors preventing other software Agencies
and Actors to instantiate them.

* Logger
* DatabaseManager
* StateMachine
* Telemetry

Are some examples of the classes (collections of data and behavior) which are
probably better off Singletons.

*Builder* pattern was used to abstract the notion of what a Command is and
so for each command that can be offered to the system by the user, it is 
contained as a class, *built* with Builder pattern, then *invoked.*

There might be a slightly better design pattern or other patterns to use
if a major refactor of the software is ever done.

# AI-Assisted

Let me get this out of the way right now.

**The software that AI generates is for the most part pure crap.**

Yes, it can write it quickly and yes, sometimes it can generate a useful idiom
that can rapidly get a prototype to work.

But for the most part -- this is **NOT HOW I WRITE SOFTWARE**.

I have tried many times to train this AI to do things a certain way, and still
the AI has only so much retention capabilities.   Rules or Directives that
*all of your software shall not pre-initialize std::string to "" since that
is already the default behavior of that class*

It's tedious and sometimes disappointing that behind all of this advanced AI, there
is still a 12-year old level of software engineering skill emerging too often.

I don't personally write code this pedantic.

Now.. Onto my point:

The upside is that a lot of the software was developed with the aid of AI.

Most of the software was co-developed with AI, specifically the **Opus-4.5** 
model and the AI Agent **Antigravity**.  This was a workable solution because
a lot of the details of the software are somewhat repetitive.  

Also, the quickness of the AI to generate the SQL and organize the Schema
of the database was very effective.

The downside is that a lot of the software was developed with the aid of AI.
And, this means the quality and effectivity of the software is in question.

AI Models of any kind are going to make many bad decisions about how to handle
data, how to compose data structures, and flow of execution.  AI models are 
terribly myopic when it comes to the architecture of the software -- they often
will focus on narrow and tight examples and fail to see the broader implications
in the architecture and design.

In plain English, AI models have no qualms about using strings when integers are
more suitable for basic data types, database fields and so on.  This leads to
a proliferation of software that is comparing characters and strings when simple
numeric (integer) based enumerations, flags, and fields are far easier and 
simpler to manage -- and debug.


# More, Later.

TBD


# Running the server

```
export MY_DBUSR=mydbusr
export MY_DBPASS=mydbpass
export MY_DB=khdb
export MY_PORT=SomeRestPort
export MY_DBHOST=127.0.0.1

# The server runs with Lisp to execute AI-Agency
# The lisp code is copied here:
mkdir -p dsl
cp ai/lisp/*.lisp dsl

screen -S KH -dm ./kh \
 --dbhost $MY_DBHOST \
 --dbusr $MY_DBUSR \
 --dbpass $MY_DBPASS \
 --dbname $MY_DB \
 --port $MY_PORT \
 --ai dsl \
 --log kh-`date +%Y-%m-%d-%H%M%S`.log

```


## Tagging

```
git submodule foreach --recursive 'git tag -a v1.0.0 -m "Version 1.0.0 tag"'
git tag -a v1.0.0 -m "Version 1.0.0 tag"
```

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



# How to Setup and Run Kepler's Horizon


This is the basic instruction for how to prepare and run the system
for Kepler's Horizon.

There are two styles of presentation.

1.  I'm going to just give the quick and dirty primer on what to do (Assumes you know your
way around a Linux/Unix system).  Skip ahead now to Dirty Primer.


2.  Else, I'm also going to walk through the more gentle and explanation style for those who need a bit more
help.

# Terms:

The **Kit** means the package of files and data that are shipped with KeplersHorizon.

1.  The binary `kh`
2.  The database files (SQL and CSV)
3.  Any Lisp files that are used by the AutonomyAgent
4.  The start-up script `run.sh`

That is the **Kit**


The keyword `EDITOR` refers to whatever editor you use.  In order to save time
this document will just use the keyword `EDITOR` to mean that editor you prefer to use.

Finally, when you see a prompt `#` it means you're `root` on the host system. Act accordingly.
When you see `$` prompt, you are *not* `root`.

## Quick and Dirty Primer

1. Make sure `mysql` is installed as well as `mysqld`
2. Create a user on mysql as follows:

The UPPERCASE are just highlighting MySQL reserved words:

```
mysql> CREATE user 'frog'@'localhost' IDENTIFIED BY 'andtoad';

mysql> GRANT ALL ON khdb.* TO 'frog'@'localhost';
```

3. In the Kit execute:

```
$ EDITOR config.in
```

If you want to add some users for your installation, add them to `user-fix.sql`

- Make the adjustments based on your user and password for the MySQL user. 
- Make the adjustments based on your choice of the database name
- Make any adjustments for new users who can access Keplers Horizon.

Create and populate the database:

This will **WIPE the database.**  Everything will be deleted for the database `khdb`.

```
$ cd db
$ make
```

Your database is setup.

4. Apache Configurations

KH works with Apache2.

In the configuration of the web server you will need to instruct Apache2 about
the Proxy for REST-ful services in the application.

The lines below are added to the configuration
Choose `PORT` (above 2000)
Replace `example.com` with the same as in `ServerName`

```
   SetEnv rate-limit 400
   SetEnv rate-initial-burst 512
   ProxyPreserveHost On
   ProxyPass        /kh/api/  http://127.0.0.1:PORT/api/
   ProxyPassReverse /kh/api/  http://127.0.0.1:PORT/api/
   RequestHeader set X-Forwarded-Proto "https"
   RequestHeader set X-Forwarded-Host  "example.com"
```

Example:

```
   ServerName keplershorizon.com
   ErrorLog ${APACHE_LOG_DIR}/error.log
   DocumentRoot /stuff/kepler
   CustomLog ${APACHE_LOG_DIR}/access.log combined
   ServerAdmin webmaster@localhost
   AssignUserID khusr www-data
   SetEnv rate-limit 400
   SetEnv rate-initial-burst 512
   <Directory /stuff/kepler>
            Options +FollowSymLinks
            Options -Indexes -MultiViews
            AllowOverride All
            Require all granted
   </Directory>

   ProxyPreserveHost On
   ProxyPass        /kh/api/  http://127.0.0.1:8080/api/
   ProxyPassReverse /kh/api/  http://127.0.0.1:8080/api/
   RequestHeader set X-Forwarded-Proto "https"
   RequestHeader set X-Forwarded-Host  "keplershorizon.com"
```


```
# a2enmod proxy proxy_http headers
# systemctl restart apache2
```

There is more you can do here (like Hardening with `fail2ban` etc..) but I will hold off and put that
in an appendix.

5. All of the content from the Kit `web/` goes into your document root.

Given your Apache DocumentRoot:

```
$ mkdir -p DocumentRoot/kh
$ cd Kit/web
$ cp -r . DocumentRoot/kh
```

6. Start the server.

The start-up script offers a couple different ways to start the application.

- It assumes you may want to work on the software!  So there's a `gdb` startup usage.
- It assumes you may want to run it with `screen` (or `tmux`) to keep it alive if the shell dies.
- Or you can run it in the foreground.

```
$ cd Kit/server
$ EDIT run.sh
```

Then start the game

```
$ bash run.sh
```

7. Jump ahead to the section Testing


## Second -- The Long Version

## What you need

1. You need a MySQL database.   

If your system already has `mysql` installed you can save a lot of time.  Let's just do some
checking here:

```
$ mysql -u root -D mysql -p
```

Do you remember the password for the `root` user of MYSQL ?  (This is not your Unix user
`root`)

You may have to become `root` on your system to login as the priviledged user on MySQL:

```
$ su
# mysql -u root -D mysql -p
```

If you got this far (at least `mysql` runs)  then that's a good sign, but there's some work to do
on your end to reset the root password of MySQL -- **again, this is NOT your Unix user `root`

If you need to install MySQL:

```
$ su
# apt-get install mysql-client  mysql-common mysql-server
```

And follow all of the instructions (**and jot down that root password you set!**)


