import _picstus

p = _picstus.Predicate("call", 1)
print p
print str(p)
print repr(p)
print

t = _picstus.Atom("call")
u = _picstus.Atom("user")
p = _picstus.Predicate(t, 1, u)
print p
print str(p)
print repr(p)
print

p = _picstus.Predicate(t, 1, "user")
print p
print str(p)
print repr(p)
print

p = _picstus.Predicate(t, 1)
print p
print str(p)
print repr(p)
print
