# Quick Start

The setup requires configuring your machine to build and run the software.

## Packages to Install

```
# apt install apache2  apache2-utils
# apt install g++-13 
# apt install ecl libecl21.2  libecl-dev  libgmp10 libgmp-dev  libgmpxx4ldbl
# apt install libgc-dev
# apt install libatomic-ops-dev
# apt install mysql-server
```


## Database Setup

```
#  mysql -u root -D mysql -p
```

Create two users:

```
mysql> create user 'khadmin'@'localhost'  identified by 'foobar';
mysql> create user 'khusr'@'localhost' identified by 'foobar;
mysql> grant all on *.* to 'khadmin'@'localhost';
mysql> grant all on khdb.* to 'khusr'@'localhost';
mysql> flush privileges;
```

## Apache setup

- If you are running this in a local VM, then note the IP address of your Ethernet Interface.
(`ifconfig -a`)
- If you are running this on a connected Internet node with a FQDN, then use the FQDN.
- Choose a `PORT_NUMBER` >= 8192 and make note of it.

Fit this inside the `Virtual Host` 

```
 # For deployments with a FQDN

 ProxyPreserveHost On
   ProxyPass        /bkhZZ/api/  http://127.0.0.1:PORT_NUMBER/api/
   ProxyPassReverse /bkhZZ/api/  http://127.0.0.1:PORT_NUMBER/api/
   RequestHeader set X-Forwarded-Proto "http"
   RequestHeader set X-Forwarded-Host  "FQDN"

 # Or..
 # For deployments without a FQDN (Local VM, for instance, WSL, etc..)

 ProxyPreserveHost On
   ProxyPass        /bkhZZ/api/  http://127.0.0.1:PORT_NUMBER/api/
   ProxyPassReverse /bkhZZ/api/  http://127.0.0.1:PORT_NUMBER/api/
   RequestHeader set X-Forwarded-Proto "http"
   RequestHeader set X-Forwarded-Host  "IP_ADDR_FROM_IFCONFIG"
```

```
# a2enmod proxy proxy_http headers
# systemctl restart apache2
```

## Server Config

Now all of the pieces are ready


1.  Create the initial database

It would be helpful to have some kind of utility script like this to re-build the database.
This script will CLOBBER the databse.  Take measures to handle preexisting data to suit.

```
# name this makefile and store in site/db/

USR=khadmin
PASS=foobar
DB=mysql -vv -u $(USR) -D mysql -p$(PASS)

DBUSR=khusr
DBPASS=foobar
DB_PROD=mysql -vv -u $(DBUSR) -D khdb -p$(DBPASS)
it:
        cat ./Init.sql | $(DB)
        cat ./Game.sql | $(DB_PROD)
        cat ./core/Load.sql | $(DB_PROD)
        cat ./modules/kh/LoadModule.sql | $(DB_PROD)
        cat ./99-fix.sql | $(DB_PROD)
        cat 99-fix.sql | $(DB_PROD)
        cat 001_add_ai_user.sql | $(DB_PROD)
```

`99-fix.sql` is just a quick way to add users you want for the game:

```
USE khdb;
DELETE FROM users;
INSERT INTO users (username, password_plain)
VALUES
('bill','whoa'),
('ted','dude'),
select * from users;
```

```
$ cd site/db
$ make
```


2. Now let's build the server 


```
$ cd site/server
$ ./makeit
```

3. Now we need to populate the web-content

```
$ cd site/web
$ mkdir -p /var/www/html/bkhZZ
$ cp -r * /var/www/html/bkhZZ
```

Test the content is available.

Open a browser to your site

URL is `http://FQDN/bkhZZ`

or

URL is `http://IP_ADDRESS/bkhZZ`

make sure you see the login page.


## Run

For a shell script to help launch:

```
# name this run.sh

export MY_DBUSR=khusr
export MY_DBPASS=foobar
export MY_DB=khdb
export MY_PORT=12210
export MY_DBHOST=127.0.0.1

mkdir -p oldlogs
rm -f kh.log
mv *.log oldlogs
logfile=kh-`date +%Y-%m-%d-%H%M%S`.log
touch $logfile
ln -s $logfile kh.log
rm -rf ./dsl
mkdir -p ./dsl
cp ai/lisp/* dsl

## If you want to run in the debugger:  

## gdb -ex "handle SIGPWR SIGXCPU SIGUSR1 SIGUSR2 nostop noprint pass" --args ./build/kh --dbhost $MY_DBHOST --dbusr $MY_DBUSR --dbpass $MY_DBPASS --dbname $MY_DB --port $MY_PORT --ai dsl --log $logfile

## Or run without the debugger

./build/kh --dbhost $MY_DBHOST --dbusr $MY_DBUSR --dbpass $MY_DBPASS --dbname $MY_DB --port $MY_PORT --ai dsl --log $logfile
```

```
$ cd site/server
$ bash run.sh
```

# Test

Remember the IP address (or FQDN) ?

1. Let's visit the URL.
2. Login with one of the fake  Users you created in the DB creation step.
3. You should see the game launch and so on.


