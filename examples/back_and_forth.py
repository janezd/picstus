import picstus
reload(picstus)
from picstus import *

a = 12

consult("python")

b = 12

print picstus.solve('python:eval("a+b", X)')

picstus.solve('python:exec("import picstus")')

picstus.solve('python:eval("id(picstus.Term)", X), python:set("sterm", X).')
print id(Term), sterm

picstus.solve('python:eval("id(type(%t))" < 12, X), python:set("sterm", X).')
print id(Term), sterm
