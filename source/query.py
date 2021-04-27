import _picstus

p = _picstus.Query("consult", ["bas.pl"])
p.next_solution()
del p

p = _picstus.Query("consult", ["bas.pl"])
p.next_solution()
del p

X = _picstus.Term()
Y = _picstus.Term()
a = _picstus.Atom("parent")
q = _picstus.Query(a, [X, Y], "user")
while q.next_solution():
    print X, Y
print "***\n"

pr = _picstus.Predicate("parent", 2)
X = _picstus.Term()
q = _picstus.Query(pr, [X, "matevz"])
while q.next_solution():
    print X
print "***\n"

X = _picstus.Term()
q = _picstus.Query("parent", [X, "matevz"])
while q.next_solution():
    print X

