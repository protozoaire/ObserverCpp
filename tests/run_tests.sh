OPT='-Wall -Wextra -std=c++17'
DST='test_observer'
SRC='test_observer.cpp'

g++ $OPT -o $DST $SRC

./$DST

rm $DST
