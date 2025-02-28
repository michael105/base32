#### base32

base32 encoder and decoder, 64bit implementation,
1kB linked statically.


Utilizing bswap and ror/rol instructions,
64bit x86 implementation.


----

This was also an experiment with branchless programming -
which showed up (in this case) to be not really advantageous.

I tried only with the encoder, 
there is a minor advantage, about 5 to 10%.
Depending on the compiler settings, code alignment,
input. (speed is here between 180MB/s - 218MB/s, depending
also on the compiler optimization setting)


However - the most simple aproach ( if then else )
showed up to be consistently performant with 0s, 02, 03.

So - I have to conclude, at least in this case,
albite there's a direct dependency chain on the branch,
there's no real advantage by programming branchless.

I leave the different codes in the file 'base32.c'.


My implementation is about 10% faster than the gnu utils one.

But. hey.

However, the gnu utils' base32 does have 47kB, dynamically linked with uClibc.

My implementations, linked statically with minilib,
come with 1kB.

Writing whole longs into the output buffer didn't show up performant,
this might be related to the missing alignment.

It should be possible to write something more performant,
but in favor of codesize I leave this, as it is.

It is just an experiment.

.. 


gcc with -O3 does a loop unrolling of the inner loop (8 loops).
It might also be possible to parallelize a little bit more
at this place. 
But in my opinion, the compactest implementation looks best.
I'd also believe, nearly noone is going to encode 
Gigabytes in base32. Would be a huge waste of memory.

---

There's one advantage with the branchless solution:
it might be cryptographically more save. (Spectre, sidechannel, ..)
Runtime and used registers should be independent of the input data.
Feeding only 0's might have the same runtime, as working with random.

Branch prediction isn't needed at all.






