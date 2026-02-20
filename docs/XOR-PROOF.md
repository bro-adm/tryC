XOR Commutativity:

lets prove that n=2 xor is commutative.

a1 ^ a2 = a2 ^ a1 ?

say a1 has more bits then a2 (intergchangable)

we will add the remaining amount of bits to the left of a2 as zeros.
now a1 = ba,b(a-1),b(a-2),...,b1 (either 0 or 1)
and a2 = ba,b(a-1),b(a-2),...,b1 (whereas the b?...b1 are orinal a2 bits and the other ba...b(?+1) are zeros)

meaning xoring a1^a2 and a2^a1 on ba...b(?+1) will be ba...b(?+1) ^ 0 = ba...b(?+1)
then from b?...b1 the options are on a1^a2 or a2^a1 either having a 1 and a 0 or a 0 and 0 or a 1 and 1 regarldess of weather a1 or a2 comes first meaning that that xor per bit is the same as well.

thus a1^a2 = a2^a1

Now for inudtion lets make an assumpation that xor is commutative for n object:

a1 ^ a2 ^ a3 ... ^ an = ...

now lets look at n+1:

XOR is associative meaning (x^y)^z = x^(y^z)

LET X = a1^a2^a3^...^an
a1 ^ a2 ^ a3 ^ .... ^ an ^a(n+1) = X ^ a(n+1)

and from the prior step we can say X ^ a(n+1) = a(n+1) ^ X
meaning:

a1^a2^a3^...^an^a(n+1) = a(n+1)^a1^a2^...^an
thus commutative

