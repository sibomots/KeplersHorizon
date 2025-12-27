# Making the Database and User


Creating a user would be ideal.

Get into mysql:

```
# mysql -u root -D mysql -p
```

From there:

```
mysql> CREATE USER 'usr'@'localhost' IDENTIFIED BY 'password';
```

Pick a `usr` name.  Keep it short.

Next, you need to run the SQL in this directory:

- `schema.sql` creates all tables
- `seed.sql` loads initial star-system + warpline data from CSV

If you need to re-run it, it would be OK to just drop the existing
database then re-create the database.

```
mysql> drop database khdb
```

Then you can re-run the `schema.sql` and `seed.sql`.

Run with local-infile enabled (needed for the CSV loads):

```
mysql --local-infile=1 -u usr -D khdb -p < schema.sql
mysql --local-infile=1 -u usr -D khdb -p < seed.sql
```

In order for your user to access the database, you need to 
grant permissions to that user:

```
GRANT ALL ON khdb.* TO 'usr'@'localhost';
```

For the `usr` name you've selected.

# Test it in the shell

```
$ mysql -u usr -D khdb -p
```

Enter the password

You should get the prompt:

```
mysql>
```



