export KH_DB_HOST=localhost
export KH_DB_USER=frog
export KH_DB_PASS='foobar'
export KH_DB_NAME=khdb
export KH_GAME_ID=1

rm -rf build
cmake -B build -S .
make -C build

