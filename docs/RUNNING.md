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
- It assumes you may want to run it with `screen` to keep it alive if the shell dies.
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


