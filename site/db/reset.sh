cat schema.sql seed.sql | mysql --local-infile=1 -D khdb -p

