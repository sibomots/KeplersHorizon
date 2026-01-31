export MY_DBUSR=borusr
export MY_DBPASS=foobar
export MY_DB=khdb
export MY_PORT=12210
export MY_DBHOST=127.0.0.1
export MALLOC_TRACE=/home/roff/gh/kh-proto/KeplersHorizon/site/server/malloc.log

rm -f kh.log
rm -rf ./dsl
mkdir -p ./dsl
cp ai/lisp/* dsl

##echo --dbusr $MY_DBUSR --dbpass $MY_DBPASS --dbname $MY_DB --port $MY_PORT
##screen -S KH -dm ./build/kh --dbusr $MY_DBUSR --dbpass $MY_DBPASS --dbname khdb --port 8081 
##echo ./build/kh --dbhost $MY_DBHOST --dbusr $MY_DBUSR --dbpass $MY_DBPASS --dbname $MY_DB --port $MY_PORT
##./build/kh --dbhost $MY_DBHOST --dbusr $MY_DBUSR --dbpass $MY_DBPASS --dbname $MY_DB --port $MY_PORT
##valgrind --leak-check=full ./build/kh --dbhost $MY_DBHOST --dbusr $MY_DBUSR --dbpass $MY_DBPASS --dbname $MY_DB --port $MY_PORT


# run in the debugger, always.
gdb -ex "handle SIGPWR SIGXCPU SIGUSR1 SIGUSR2 nostop noprint pass" --args ./build/kh --dbhost $MY_DBHOST --dbusr $MY_DBUSR --dbpass $MY_DBPASS --dbname $MY_DB --port $MY_PORT --ai dsl
