USR=frog
PASS=foobar
DB=mysql -vv -u $(USR) -D mysql -p$(PASS) 

DBUSR=borusr
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
