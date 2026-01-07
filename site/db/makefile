DBUSR=frog
it:
	echo "create db"
	cat 0-schema.sql 1-milieu.sql 2-seed.sql 3-help_load.sql 4-seed_milieu.sql 5-constraints.sql | mysql  -u $(DBUSR) -D mysql -p

fix:
	cat 99-fix.sql | mysql -vv -u $(DBUSR) -D khdb -p

