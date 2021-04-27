class BoolList(list):
    def __init__(self, b):
        list.__init__(self, [{}][:bool(b)])
        
    def __str__(self):
        return str(bool(self))
    
    def __repr__(self):
        return str(bool(self))


def yieldSolutions_native(query, terms):
    yield dict([(t.name, t.toPython()) for t in terms])
    while query.next_solution():
        yield dict([(t.name, t.toPython()) for t in terms])

def yieldSolutions(query, terms):
    yield dict([(t.name, t) for t in terms])
    while query.next_solution():
        yield dict([(t.name, t) for t in terms])

import re
re_variable = re.compile(r"\W([A-Z_]\w*)")
re_quotes = re.compile(r"""(?P<quote>('|")).*?[^\\](?P=quote)""")


def solutions(s, a_terms=None, **kwds):
    if a_terms == None: # empty lists should not go here!
        if s[-1] == ".":
            s = s[:-1]
            
        variables = []
        for v in re_variable.findall(re_quotes.sub("", s)):
            # by always appending _ we ensure that each _ is unique
            if v == "_" or not v in variables:
                variables.append(v)

        call_pred = Predicate("call", 1, "prolog")
        terms = [Term(name=i) for i in variables]
        goal = Term(s, terms)
        query = Query(call_pred, [goal])

        terms = filter(lambda t:t.name and t.name[0] != "_", terms)

    else:
        terms = []
        termdict = {}
        inSolution = []
        for aa in a_terms:
            if getattr(aa, "__class__", None) == _Faker__PostponedAtom:
                aa = aa.name
            if type(aa) == str:
                if not aa or aa == "_":
                    terms.append(Term())
                elif aa[0].isupper() or aa[0]=="_":
                    if not termdict.has_key(aa):
                        term = termdict[aa] = Term(name=aa)
                    else:
                        term = termdict[aa]
                    terms.append(term)
                    if aa[0] != "_":
                        inSolution.append(term)
                else:
                    terms.append(Term(aa))
            elif type(aa) == Term:
                terms.append(aa)
                if aa.is_variable() and getattr(aa, "name", "")[:1] not in ["", "_"]:
                    inSolution.append(aa)
            else:
                terms.append(Term(aa))

        query = Query(s, terms)
        terms = inSolution
        
    if terms:
        if not query.next_solution():
            return BoolList(False)
        else:
            if kwds.get("convertToPython", True):
                return yieldSolutions_native(query, terms)
            else:
                return yieldSolutions(query, terms)
    else:
        return BoolList(query.next_solution())


def findall(s, a_terms = None, **kwds):
    return list(solutions(s, a_terms, **kwds))

def exists(s, a_terms = None, **kwds):
    return bool(solutions(s, a_terms, **kwds))

def solve(s, a_terms = None, **kwds):
    for solution in solutions(s, a_terms, **kwds):
        return solution
    
def consult(filename):
    Query("consult", [filename]).next_solution()


class _Faker__PostponedAtom:
    def __init__(self, name):
        self.name = name
        
    def __call__(self, *a, **kwds):
        return solutions(self.name, a, **kwds)

class __Faker:
    def __getattr__(self, s):
        if s and s[0].isupper():
            return s
        else:
            return _Faker__PostponedAtom(name=s)


import sys, imp

# This annoyance is needed since Python 2.5 on Windows doesn't accept extension .dll
# for module anymore, while SICStus foreign resource MUST be a .dll...
_picstus = sys.modules.get("_picstus")
if not _picstus:
    if sys.platform == "win32":
        dllName = "python.dll"
    else:
        dllName = "python.so"  # hope 'so' ...

    import os
    if os.path.exists(dllName):
        _picstus = imp.load_dynamic("_picstus", dllName)
    else:
        SP_PATH = os.environ.get("SP_PATH")
        if not SP_PATH:
            raise ImportError, "Cannot locate Sicstus; please set the SP_PATH environment variable"
        if SP_PATH[-1] not in ["/", "\\"]:
            SP_PATH += "/"
        if not os.path.exists(SP_PATH+dllName):
            raise ImportError, "Cannot locate python.dll in Sicstus' library directory I got from SP_PATH (%s)" % SP_PATH
        _picstus = imp.load_dynamic("_picstus", SP_PATH+dllName)

from _picstus import *

_ = "_"

sp = __Faker()

if __name__=="__main__":
    consult("bas.pl")

    print "\n?- parent(adam, abel)"
    print sp.parent("adam", "abel")

    print "\n?- parent(adam, _)"
    print sp.parent("adam", _)

    print "\n?- parent(abel, _)"
    print sp.parent("abel", _)

    print "\n?- parent(X, Y)"
    for i in sp.parent(sp.X, sp.Y):
        print i

    print "\n?- parent(X, X)"
    for i in sp.parent(sp.X, sp.X):
        print i
        
    print "\n?- parent(X, X)"
    for i in sp.parent("X", "X"):
        print i

    print "\n?- parent(X, abel)"
    for i in sp.parent(sp.X, "abel"):
        print i

    print "\n?- parent(X, abel)"
    for i in sp.parent(sp.X, sp.abel):
        print i

    if sp.parent(sp._, sp.abel):
        print "abel has parents"
    else:
        print "abel has no parents"

    if sp.parent(sp._, sp.adam):
        print "adam has parents"
    else:
        print "adam has no parents"

    print "\n?- sp.parent(X, sp.adam)"
    X = Term()
    for t in sp.parent(X, sp.adam):
        print X
        
    print "\n?- parent(_, _)"
    print sp.parent(_, _)

    print "\n?- parent(abel, adam)"
    print sp.parent(sp.abel, sp.adam)

    print "\n?- parent(adam, abel)  (for)"
    for i in sp.parent(sp.adam, sp.abel):
        print i

    print "\n?- parent(adam, abel)"
    print sp.parent(sp.adam, sp.abel)

    print '\n?- solutions("parent(X, Y).")'
    for t in solutions("parent(X, Y)."):
        print t

    print '\n?- solutions("parent(X, _).")'
    for t in solutions("parent(X, _)."):
        print t

    print '\n?- solutions("parent(abel, X).")'
    for t in solutions("parent(abel, X)."):
        print t

    print '\n?- solutions("parent(_, _).")'
    print solutions("parent(_, _).")

    print '\n?- solutions("parent(X, Y)")'
    for t in solutions("parent(X, Y)"):
        print t

    print '\n?- solutions("parent(adam, abel)")'
    print solutions("parent(adam, _)")
   
    print '\n?- solutions("parent(adam, abel)  (for)")'
    for i in solutions("parent(adam, abel)"):
        print i

    print '\n?- solutions("parent(abel, adam)  (for)")'
    for i in solutions("parent(abel, adam)"):
        print i

    print '\n?- solutions("parent(abel, _)")'
    print solutions("parent(abel, _)")
   
    print '\n?- solutions("parent(_, _)")'
    print solutions("parent(_, _)")

    print '\n?- solutions("parent(_, _)")  (for)'
    for i in solutions("parent(_, _)"):
        print i

    print '\nif solutions("parent(abel, X)"):'
    if solutions("parent(abel, X)"):
        print 'solutions("parent(abel, X)") succeeds!'
        
    print '\n?- findall("parent(X, Y)")'
    print findall("parent(X, Y)")

    print '\n?- findall("parent(X, abel)")'
    print findall("parent(X, abel)")

    print '\n?- findall("parent(_, abel)")'
    print findall("parent(_, abel)")

    print '\n?- findall("parent(abel, _)")'
    print findall("parent(abel, _)")

    print '\n?- exists(parent("adam, abel"))'
    print exists("parent(adam, abel)")
    
    print '\n?- exists(parent("abel, adam"))'
    print exists("parent(abel, adam)")

    print '\n?- exists(parent("abel, X"))'
    print exists("parent(abel, X)")

    print '\n?- exists(parent("abel, _"))'
    print exists("parent(abel, _)")

    print '\n?- exists(parent("_, abel"))'
    print exists("parent(_, abel)")

    print '\n?- solutions("parent", [X, "abel"])'
    a = Term(name="X")
    for t in solutions("parent", [a, "abel"]):
        print t

    print '\n?- solutions("parent", [X, "abel"])'
    for t in solutions("parent", [a, "abel"]):
        print a

    for t in solutions("parent(adam, _)"):
        print "adam has children"
        break
    else:
        print "adam has no children"

    for t in solutions("parent(abel, _)"):
        print "abel has children"
        break
    else:
        print "abel has no children"
        
    print '\n?- solutions("parent", [a, Term(name="X")])'
    a = Term()
    for t in solutions("parent", [a, Term(name="X")]):
        print t

    print '\n?- sp.parent(Y, "abel")'
    for t in sp.parent(sp.Y, "abel"):
        print t

    print '\n?- sp.parent(Y, "abel")'
    Y = Term(name="Y")
    for t in sp.parent(Y, sp.abel):
        print Y

    print '\n?- sp.parent(Y, "abel")'
    Y = Term()
    for t in sp.parent(Y, sp.abel):
        print Y

    print '\n?- parent(Y, abel)'
    abel = "abel"
    parent = sp.parent
    Y = Term(name="Y")
    for t in parent(Y, abel):
        print Y


    print '\n?- if solutions("parent(X, abel)")'
    if solutions("parent(X, abel)"):
        print "abel has parents"
    else:
        print "abel has no parents"

    print '\n?- if solutions("parent(X, adam)")'
    if solutions("parent(X, adam)"):
        print "adam has parents"
    else:
        print "adam has no parents"

    print '\n?- findall("t(X)")'
    try:
        print findall("t(X)")
        print "error: should throw an exception!"
    except SicstusError:
        print "exception thrown OK"
        