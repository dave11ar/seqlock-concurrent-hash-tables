rm -rf external/

wget https://github.com/boostorg/sync/archive/master.tar.gz -P external/boost/sync

tar -xzvf external/boost/sync/master.tar.gz --strip-components 1 -C external/boost/sync/
rm external/boost/sync/master.tar.gz
