## Thoughts

There is only partial evaluation in a function if it has a comptime parameter.
At least in the sense where comptime and runtime control flow are interleaved.

I feel like functions that do take a comptime parameter and ones that don't are
sort of each others opposites in what sort of code you want to generate for them.
For comptime functions you generate code to run for all the comptime stuff and
output 'emit' points for all the runtime code.
But for regular functions you generate code for all the runtime code and output
'eval' points for all the expressions that should be run at compile time.
