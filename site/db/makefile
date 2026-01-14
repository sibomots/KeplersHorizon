DBUSR=frog
DBPASS=foobar
DB=mysql -vv -u $(DBUSR) -D mysql -p$(DBPASS) 
it:
	cat ./Game.sql | $(DB)
	cat ./core/Load.sql | $(DB)
	cat ./modules/kh/LoadModule.sql | $(DB)
	cat ./99-fix.sql | $(DB)
	cat 99-fix.sql | $(DB)

