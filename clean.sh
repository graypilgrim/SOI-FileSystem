./fs -destroy
set +x

find ./ -name 'small_*' -exec rm {} \;
find ./ -name 'big_*' -exec rm {} \;
