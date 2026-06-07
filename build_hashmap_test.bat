@echo off

clang -Wall -Wextra -Werror -Isrc test/hashmap.test.c -o hashmap.test.exe
clang -Wall -Wextra -Werror -Isrc -Iext -g -gcodeview test/hashmap.fuzz.c -o hashmap.fuzz.exe -Wl,/DEBUG
clang -Wall -Wextra -Werror -Isrc -Iext -O2 -march=native test/hashmap.fuzz.c -o hashmap.opt.fuzz.exe -Wl,/DEBUG
