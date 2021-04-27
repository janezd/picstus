import _picstus

isS = ["variable", "integer", "float", "atom", "compound", "list", "atomic", "number"]

def printout(t):
    print "pure print: ", t
    print "repr: ", repr(t)
    print "type: ", t.getType()
    print ", ".join([i + ": " + str(getattr(t, "is_"+i)())[0] for i in isS])

print "\n\n*** Construction ***"

print "\nTerm()"
t = _picstus.Term()
printout(t)

print "\nTerm(15)"
t = _picstus.Term(15)
printout(t)
print int(t)
print t+7
print t+7.0

print "\nTerm('test')"
t = _picstus.Term("test")
printout(t)
print str(t)
#print t + "2"

print "\nTerm(15.0)"
t = _picstus.Term(15.0)
printout(t)
print float(t)
print t+7
print t+7.0

print "\nTerm('parent(irena, matevz).', [])"
t = _picstus.Term("parent(irena, matevz).", [])
printout(t)
print t[0]
print t[1]
print list(t)
print len(t)

print "\nTerm('parent', 2)"
t = _picstus.Term("parent", 2)
printout(t)

print "\nTerm([1, 2, 3])"
t = _picstus.Term([1, 2, 3])
printout(t)
print t[2]
print t[0]
#print len(t)
print list(t)
print "\n\n*** Assignment ***"

print "\nassign()"
t = _picstus.Term()
t.assign()
printout(t)

print "\nassign(15)"
t = _picstus.Term()
t.assign(15)
printout(t)

print "\nassign('test')"
t = _picstus.Term()
t.assign("test")
printout(t)

print "\nassign(15.0)"
t = _picstus.Term()
t.assign(15.0)
printout(t)

print "\nassign('parent(irena, matevz).')"
t = _picstus.Term()
t.assign("parent(irena, matevz).")
printout(t)

print "\nassign('parent', 2)"
t = _picstus.Term()
t.assign('parent', 2)
printout(t)

print "\nassign([1, 2, 3])"
t = _picstus.Term()
t.assign([1, 2, 3])
printout(t)

t = _picstus.Term("parent(m, i)", [])
print t[0]
print t[1]


X = _picstus.Term()
Y = _picstus.Term()
t = _picstus.Term("parent(john, X)", [X])
u = _picstus.Term("parent(Y, eve)", [Y])
print _picstus.unify(t, u)
print X
print Y
print X<Y, X>Y, X==Y

X, Y, T = _picstus.Term(), _picstus.Term(), _picstus.Term()
t = _picstus.Term("parent(T, X)", [T, X])
u = _picstus.Term("parent(Y, eve)", [Y])
v = _picstus.Term("parent(john, eve)", [])
print _picstus.unify(t, u)
print _picstus.unify(v, u)
print X
print Y
print T

X, Y, T = _picstus.Term(), _picstus.Term(), _picstus.Term()
t = _picstus.Term("parent(Y, X)", [Y, X])
u = _picstus.Term("parent(Y, eve)", [Y])
v = _picstus.Term("parent(john, eve)", [])
print _picstus.unify(v, u)
print X

v = _picstus.Term("parent(john, eve)", [])
u = _picstus.Term("parent(john, andrew)", [])
print _picstus.unify(u, v)