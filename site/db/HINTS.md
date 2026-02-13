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

Next, you need to run the SQL in this directory and in `systems/`

- `newschema.sql` creates all tables
- `newmilieu.sql` creates all tables for the Milieu
- `seed.sql` loads initial star-system + warpline data from CSV
- `systems/`  CSV and SQL for loading the Milieu

If you need to re-run it, it would be OK to just drop the existing
database then re-create the database.

```
mysql> drop database khdb
```

Then you can re-run all of the SQL.  It would be recommended to 
wipe the database if you're in the midst of testing the software
(contributing, etc.. ) between tests.

Run with local-infile enabled (needed for the CSV loads):

```
$ mysql --local-infile=1 -u usr -D khdb -p < newschem.sql
$ mysql --local-infile=1 -u usr -D khdb -p < newmileu.sql
$ mysql --local-infile=1 -u usr -D khdb -p < seed.sql
$ pushd systems
$ mysql --local-infile=1 -u usr -D khdb -p < seed_milieu.sql
$ popd
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

```
mysql> show tables;

+------------------------+
| Tables_in_khdb         |
+------------------------+
| combat_orders          |
| combat_state           |
| drafts                 |
| game_events            |
| game_seats             |
| games                  |
| codex_entries          |
| hexes                  |
| rooms                  |
| sessions               |
| ships                  |
| sightings              |
| star_systems           |
| system_anomalies       |
| system_asteroid_belts  |
| system_facilities      |
| system_codex_rumors    |
| system_moons           |
| system_planets         |
| system_populations     |
| system_resources       |
| system_species         |
| system_stars           |
| telemetry_queue        |
| users                  |
| warpline_hexes         |
| warplines              |
+------------------------+
27 rows in set (0.00 sec)

mysql> exit
```


