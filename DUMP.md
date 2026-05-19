I just ran into a problem; types may be dependent on const code.
This means that you cannot first typecheck everything and then run const code.
You have to interleave the two where necessary.

Typecheck a bit of code, then evaluate it, which results in a type, which you can use to typecheck something else, etc...
I knew this, but I did not adapt my compiler to this. Why not?!?!
I feel stupid about this :( but not defeated. I can fix it.

1. tokenize
1. parse
1. typecheck + const evaluation
1. evaluate program
