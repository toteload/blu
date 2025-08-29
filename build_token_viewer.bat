clang++ -g -gcodeview -Wall -Wextra -std=c++17 -D_CRT_SECURE_NO_WARNINGS -c -Isrc src\tools\tokenviewer.cc -o tokenviewer.o
clang++ -g -gcodeview -Wall -Wextra -std=c++17 -D_CRT_SECURE_NO_WARNINGS -c -Isrc src\utils\stdlib.cc -o stdlib.o
clang++ -g -gcodeview -Wall -Wextra -std=c++17 -D_CRT_SECURE_NO_WARNINGS -c -Isrc src\tokenize.cc -o tokenize.o
clang++ -g -gcodeview -Wall -Wextra -std=c++17 -D_CRT_SECURE_NO_WARNINGS -c -Isrc src\toteload.cc -o toteload.o

clang++ -g tokenviewer.o stdlib.o tokenize.o toteload.o -o tokenviewer.exe
