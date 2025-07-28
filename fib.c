#include <stdint.h>
#define true 1
#define false 0
typedef uint32_t b32;
typedef int8_t i8;
typedef uint8_t u8;
typedef int16_t i16;
typedef uint16_t u16;
typedef int32_t i32;
typedef uint32_t u32;
typedef int64_t i64;
typedef uint64_t u64;
typedef float f32;
typedef double f64;

typedef struct slice { void *data; i64 len; } slice;

i32 fib(i32 n);
i32 sum(slice xs);
i64 count(i32 needle, slice haystack);
i32 main();
i32 fib(i32 n){
if ((n <= 1)) {
return 1;
} else {
return (fib((n - 1)) + fib((n - 2)));
}
}
i32 sum(slice xs){
i32 s = ((i32)0);
slice a0001 = xs;
for (i64 a0000 = 0; a0000 < a0001.len; a0000 += 1) {
i32 x = ((i32*)a0001.data)[a0000];
(s += x);
}
return s;
}
i64 count(i32 needle, slice haystack){
i64 s = ((i64)0);
slice a0003 = haystack;
for (i64 a0002 = 0; a0002 < a0003.len; a0002 += 1) {
i32 x = ((i32*)a0003.data)[a0002];
if ((x == needle)) {
(s += 1);
}
}
return s;
}
i32 main(){
i32 i = ((i32)1);
i32 x = ((2 + (((5 * 16) / 4) * 35)) + i);
while (true) {
if ((((i + (2 * 3)) == 10) && true)) {
break;
}
i = (i + 1);
continue;
}
return fib(10);
}
