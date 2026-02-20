a^b = c

c^a = b
c^b = a

let n = max_len(a, b) // bitwise
again all pad the smaller one with zeros to left

now for i=n-1 to i=0

c(i):
* 0 -> means a(i) = b(i) (either 0 or 1)
* 1 -> a(i) or b(i) is 1 and the other is 0

meaning:
* if c(i) is 1 and c(i)^a(i) is 0 then a(i) is 1 and b(i) is 0
* if c(i) is 1 and c(i)^a(i) is 1 then a(i) is 0 and b(i) is 1
* if c(i) is 0 and c(i)^a(i) is 0 then a(i) is 0 and b(i) is 0
* if c(i) is 0 and c(i)^a(i) is 1 then a(1) is 1 and b(i) is 1

as we can see the c(i)^a(i) allows us to see what b(i) was and it also shows it actually is equals to b(i) always
the same thing applies to c(i)^b(i)
