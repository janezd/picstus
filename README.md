Picstus: Two-way Python to Prolog Interface
===========================================

Picstus is a two-way interface between Python and Sicstus Prolog. It allows using Sicstus from Python and vice versa, where one language is seen as a module of the other. Code in the two languages can even mutually call one another to an arbitrary depth.

> **Disclaimer**: This is a very old toy project I found on my disk. Judging by this documentation, it was compatible with Python 2.3 on (probably some old) Windows. I clearly remember this actually worked. The code may not be nice (I imagine I'm a better programmer now), but some concepts are definitely interesting, so I'm archiving this on github. 

Requirements
------------

Picstus requires Python and Sicstus (or at least their crucial parts). I run it on Windows, but it shouldn't be difficult to port it to Linux. It is not picky about the version of Python. Regarding Sicstus, version 3.8 won't work and 3.12 does; I have no idea about the versions in between.

The Windows binary file provided on this page is compiled and linked against Python 2.3 and Sicstus 3.12. To work with another version of Python, the module needs to be rebuild. I used Visual C++ 6.0, but other compilers should work as well (possibly with minor adjustments). I have no idea about the constancy of Sicstus API, so I'd appreciate information whether rebuilding is needed for different versions Sicstus, too.

Download
--------

**Binary installation for Windows + Python 2.3 + Sisctus 3.12** (PicstusInstallation.exe): the preferred installation - run it and it should do the job. It hasn't been tested on any other machine than mine, so positive and negative feedbacks are welcome.

**Binary files (PicstusBinary.zip):** use if the installation file above doesn't work. The archive contains the same stuff as the installation, except that you have to copy the files manually: `picstus.py` goes to the Python's subdirectory `lib/site-packages`, `python.pl` and `python.dll` go to the Sicstus' subdirectory `library`. You will also need to check whether the Sicstus installer has set environment variable `SP_PATH` to contain the Sicstus' base directory; set it yourself if needed. If Picstus complains that it cannot find sprt312.dll, add its directory to`PATH`.

**Source distribution:** use this if you cannot use the provided binaries due to mismatching versions. Compile `picstus.cpp` and `python_glue.c`, link it with `pythonXX.lib` (say `python23.lib`) and `sprtYYY.lib` (say `sprt312.lib`) and name the resulting file `python.dll` (or `python.so` on Linux). Then copy the files as described above. Project files for VS 6.0 are provided, but need to be modified to link with the right versions. One more nasty detail: for the time of compilation, replace the spaux.c in Sictus' directory `include` with the replacement provided in the archive. There should be a way around this, but the corresponding `#define`s are not described in the Sicstus API documentation.


# Usage

This document first describes the predicates for calling Python from Prolog. Then follows the richer interface for the opposite direction, [calling Prolog from Python](#prologToPython). The last part, which will probably be less interesting for most readers, describes the [low-level structures](#LowLevel) which are seen from Python and are common to both interfaces.

Calling Python from Prolog
--------------------------

Prolog to Python interface is implemented in a module which is surprisingly called `python`. To use it, import it with the usual `consult(python).`

### Basic Predicates

The module contains a few basic predicates and a few utility predicates that  are based on the basic ones. This section describes the former.

`python:exec(+statement)`
: Executes an arbitrary python statement given as an atom or a string. The may have have side-effects, such storing something in a Python variable. Printouts may or may not be visible, depending on the type of Sicstus shell. The statement can also be a function call, but it cannot return any results to Prolog. 
    
  ```prolog
  | ?- python:exec("a = 12").
  yes
  | ?- python:exec("b = a * 3").
  yes
  | ?- python:exec("print a, b").
  12 36 yes
  ```

  We will typically want to include values of Prolog terms in the expression. For instance, we may want to assign the value of term `X` to a Python variable `a`. This is one of the ways:

  ```prolog
  | ?- X=2, python:exec("a = %v" < [X]).
  X = 2 ?
  yes
  | ?- python:exec("print a").
  2
  yes
  | ?- python:exec("print (%v + %v) * %v" < [2, 3, 4]).
  20
  ```
    
  The details, among with a few other control characters like `%v` are given in the section on Data conversion (below).

`python:eval(+expression, ?X)`
: Evaluates arbitrary Python expression given as a string or an atom. Prolog terms can also be inserted, like shown above (and described in the section on [data conversion](#DataConversion)). The expression can be, for example, a numerical expression (such as `2 * 2`) or, more useful, a variable or a function call. The result is unified with`X`.
    
  ```python
  | ?- python:eval(a, X).
  X = 12 ? ;
  no
  | ?- python:eval("a * a", 143).
  no
  ```
    
  Result of a function can be an atom, string, integer, float or a term. The latter happens if the function explicitly constructs a term (by using `picstus.Term`, see the documentation for the Python part) or when the function returns a list or a tuple. For instance, Python's function
    
  ```python
  def f():
      return 1, 2
  ```
    
  returns a tuple with elements 1 and 2, which is returned as a Prolog list. Unification works as expected. 
    
  ```prolog
  | ?- python:eval("f()", X).
  X = [1,2] ? ;
  no
  | ?- python:eval("f()", [1, X]).
  X = 2 ? ;
  no
  | ?- python:eval("f()", [42, X]).
  no
  ```

`python:set(+variable, +value)`
: Converts value to a Python value, which should be a float, integer, string or a list of any of these (including lists of list), and stores them into Python variable. It is not possible to store Prolog terms into Python variables this way (see comments below). The variable can be given as an atom or a string.
    
  ```prolog
  | ?- python:set(a, 12).
  yes
  ```
    
  For a more complicated example, see the following.
  
  ```python
  | ?- python:set(l, [0, 1, 2, 3, 4]).
  yes
  | ?- python:set('l[3]', 'something else').
  yes
  | ?- python:eval(l, X).
  X = [0,1,2,'something else',4] ?
  yes
  | ?- python:exec("print l").
  [0, 1, 2, 'something else', 4]
  yes
  ```
  
  Note that the first argument may always be an atom or a string, so it doesn't matter what quotes we use; when the Python expression or statement looks like a Prolog atom (like variable name `l` in the above example), we even don't need the quotes.

`python:call(+function, +list-with-arguments, ?result)`
: Calls the given function with the provided arguments. Function name can be given as an atom or a string. Result can be a single object or a tuple; in the latter case, it is converted to a Prolog list. If the function returns nothing (`None`), the result in a Prolog atom `none`. For example, to call a Python function `sumtwo` defined as
    
  ```python
  def sumtwo(a, b):
      return a + b
  ```
  
  one would say
  
  ```prolog
   | ?- python:call(sumtwo, [1, 2], X).
   X = 3 ? ;
   no
   | ?- python:call(sumtwo, ["Picstus ", "rocks!"], _X), format(_X, []).
   Picstus rocks!
   yes
   ```

## Utility Predicates

Module python also contains several predicates that are defined in Prolog. They implement a few common operations.

`python:execfile(+filename)`
: Executes a Python script. This is predicate is equivalent to (and is actually implemented as)
    
  ```prolog
  execfile(F) :- python:call(execfile, [F], _)
  ```

`python:import(+modulename)`
: Imports module `modulename`. Implementation is somewhat nasty since Python's `import` statement is a statement, not function, and since the function `__import__` returns a module instead of putting it  into the namespace. So the implementation constructs a string  `import <modulename>` and gives it to Python's function `exec`, which is executed through Prolog predicate `exec`.

  ```prolog
  import(M) :-
      exec("exec('import %%s' %% '%s')" < M).
  ```

`python:import(+modulename)`
: Similar to the above, but equivalent to Python's `from <modulename> import *`.

### Data Conversion

#### Control characters for `python:exec` and `python:eval`

In predicates `python:exec/1` and `python:eval/2` the first argument can be constructed using the `<` operator, where the right-hand side is a list with the terms to replace the corresponding control characters in the string on the left-hand side.

For convenience, if there is only one control character, and the term we want to substitute it with is not a list, we can give a single term instead of a list.

```prolog
| ?- python:exec("print 6 * %v" < 7).
42
yes
```

Note, however, that when a list is given, it cannot be treated as a single argument for the single control character; we need to enclose the list in a list.

```prolog
| ?- python:exec("print %v" < [1, 2, 3]).
! 'too many arguments for the format string'
| ?- python:exec("print %v" < [[1, 2, 3]]).
[1, 2, 3]
yes
```

Sadly, the same goes for strings, which cannot be distinguished from Prolog lists. We can either put them in square brackets or pass them as atoms.

Here's a list of all control characters.

`%i`
: The value of term, which should be an integer or float, is inserted into the string. In the following example
    
  ```prolog
  python:exec("print(%i + %i) * %i" < [2, 3, 4]).
  ```

  the string is changed to `print (2 + 3) * 4` and passed to Python. Floats are rounded down.
  
  ```prolog
  | ?- python:exec("print %i" < 2.5).
  2
  yes
  ```

`%f`
: Similar to the above, except that the value must be a float.

`%s`
: Again similar to the above, except that the term should now be an atom or a string. The `string/1` predicate, described below, is not needed: all lists are treated as string. For convenience, `%s` also accepts integers and floats, which are then treated as if the control character was `%i` or `%f`. Hence, the following example

  ```prolog
  | ?- python:exec("print (%s + %s) * %s" < [2, 3, 4]).
  ```

  works exactly like the one above that uses `%i`.

`%v`
: The value of the term is converted to Python (see the rules below) and  assigned to a temporary Python variable; `%v` is replaced by the variable's name. Hence, the above example is equivalent to the following code in Python (names of the temporary variables can be different):
    
  ```python
  _sp_ps_15 = 2
  _sp_ps_16 = 3
  _sp_ps_17 = 4
  print(_sp_ps_15 + sp_ps_16) * _sp_ps_17
  ```
    
  Equivalent terms (with equivalency defined as Prolog's `==` operator understands it) are put into the same variable. Here's an example:
    
  ```prolog
  | ?- X=[1, 2], python:exec("%v.append(3); t = %v; print t" < [X, X]).
  [1, 2, 3]
  X = [1,2] ?
  yes
  ```
    
  The code translates to Python
    
  ```python
  as _sp_ps_22 = [1, 2]
  _sp_ps_22.append(3)
  t = _sp_ps_22
  print t
  ```
    
  and not

  ```python
  _sp_ps_22 = [1, 2]
  _sp_ps_23 = [1, 2]
  _sp_ps_22.append(3)
  t = _sp_ps_23
  print t
  ```
    
  It is also instructive to see that there is no feedback: term `X` doesn't change to [1, 2, 3]. (A Prolog programmer would of course find changing the term unacceptable, while a Python programmer without much experience in Prolog could expect `X` to change.)

`%t`
: Like `%v`, except that the term is not converted to Python's native type, but left as an instance of [`picstus.Term`](#Term). This way we can pass more complex structures to Python expressions and parse them in Python.

#### Conversions of Prolog Terms to Python types

The terms given as arguments to the `python:call/3` predicate, the second argument of `python:set` predicate and also the terms that are put into temporary variables through `%v` control character described above, are converted into Python objects of corresponding types: Prolog integers and floats become Python integers and floats, atoms become strings and Prolog lists become Python lists, where each element is, recursively, converted. Atom `none` becomes a Python's `None`. Compound terms (with exception of lists and the two predicates described below) remain Prolog objects and the Python code can access such objects as described below, in the section on [low level structures](#LowLevel).

Here are a few predicates that can be used to control the conversion.

`string`
: Prolog has no strings, but uses lists of integers instead. This is no problem for Prolog, predicates like `write` expect a list and treat it as a string. But it is a problem for Picstus, since it cannot know when the list should be converted to a string and when not; to be on the safe side, it leaves a list as a list.
    
  ```prolog
  | ?- python:set(a, "this is a string").
  yes
  | ?- python:exec("print a").
  [116, 104, 105, 115, 32, 105, 115, 32, 97, 32, 115, 116, 114,
  105, 110, 103]
  yes
  ```
    
  To force a conversion to a string, wrap the string into predicate `string`.
    
  ```prolog
  | ?- python:set(a, string("this is a string")).
  yes
  | ?- python:exec("print a").
  this is a string
  yes
  | ?- python:set(a, string([65, 66, 67])).
  yes
  | ?- python:exec("print a").
  ABC
  yes
  ```
    
  There is another, better way to pass strings to Python: pass them as atoms.
    
  ```prolog
  | ?- python:set(a, 'this is a string').
  this is a string
  yes
  ```

`term`
: To prevent conversion of the types like integers and atoms that convert automatically into Python objects, use predicate `term`.
    
  ```prolog
  | ?- python:exec("print %v, type(%v)" < 42).
  42
  yes
  | ?- python:exec("print %v, type(%v)" < term(42)).
  42
  yes
  ```

  In the second case, the object still printed out as if it was an integer, but printing out its type reveals that it remained a Prolog term (`picstus.Term`). The equivalent effect would be achieved by using `%t` instead of `%v` and omitting the predicate `term`.

  Storing such terms in Python is not a good idea, as we will explain later.

none
: `none` is an atom that represents Python's constant `None`.


### Memory Management Issues

For a more active use - throwing Prolog's and Python's data back and forth - we need to be aware that (as will be explained with more technical details later) Python and Prolog have drastically different memory management. In Prolog, all terms created inside the query are thrown away when the query is done. Calling any of the above predicates is considered a query, hence a term stored in a Python variable can be dead immediately upon return. The following code is OK (and also tells something about how well unification works in picstus!)

```prolog
python:set("a", X), X=12, python:exec("print a").
```

while the following has a problem:

```prolog
| ?- python:set(a, term(12)).
yes
| ?- python:exec("print a").
! 'term is dead (it belonged to an ended query)'
```

The term 12 from the first list line dies as soon as the query is finished. A Python variable `a` still holds a reference to it, not knowing what has happened outside (really, the Picstus interface code has *no way* of knowing whether `python:set` was called like in the former or in the latter example). When we request `python:exec("print a")`, it nicely informs us that the term is gone. Note that the trick through which picstus determines whether the term is still with us or not, may not be foolproof, so it may be possible to devise ways to get erroneous output or maybe even crash picstus by deliberately storing some deeply nested terms.


### Examples

The interface to Python is useful for two purposes. Python is an excellent scripting language, so we can use it to implement the procedural parts which would be irksome in Prolog. Second, Python delivers a vast collection of modules for any purpose we may think of. In this section, we are gonna show a few.

How do we read a web page in Prolog through Picstus? Trivial.

```prolog
| ?- python:import(urllib).
yes
| ?- python:eval("urllib.urlopen('%s').read()" < 'http://www.ailab.si', X).
```

Note that we passed the URL as an atom. If we did is as a string, we'd need to enclose it in square brackets (see the section on data conversion).

One of the most irritating thing in typical Prolog implementations is a complete lack of any cryptographic library. (This is of course not true, but "*lying is a skill like any other, and if you want to maintain a level of excellence you have to practice constantly.*"; Garak, Star Trek DS 9). Let us show how to compute a MD5 digest of the page we got in the previous example.

```prolog
| ?- python:import(md5).
yes
| ?- python:exec("m = md5.new()").
yes
| ?- python:eval("urllib.urlopen('%s').read()" < 'http://www.ailab.si', _X), python:call("m.update", [_X], _).
yes
| ?- python:call("m.hexdigest", [], X).
X = d49e044b207d82f235010e0bf839dd7a ?
yes
```

The module that takes care of that is `md5`. We constructed an instance of class `md5` and stored it to a Python variable `m`. We fed the web page (`_X`) to the `m`'s method `update(<string<)` and called `m.hexdigest()` to get the digest. Method `update` returns no result which, in Python, means that it returns `None`. Not being interested in that, we assigned it to `_`.

For a more useful example, let us write a predicate which get all the links from the given web page. Here's the definition.

```prolog
:- consult(python).

:- python:import(urllib).
:- python:import(re).

getHrefs(URL, Hrefs) :-
    python:eval("urllib.urlopen('%s').read()" < [URL], Page),
    python:call("re.findall", ['(?i)hrefs*=s*"([^"]*)"', Page], Hrefs).
```

We have already seen the module `urllib` and how to call it. Module `re` is about regular expressions. The one we constructed first says it's case insensitive (`(?i)`), then follows a `href`, possibly some whitespace, followed by `=` and maybe some more white space and a quote. Part `[^"]*` represents any number of any characters except a quote; we closed this part of the string into parentheses to define it as a group. Finally, we have the closing quote.

We have passed the regular expression as an atom, not a string. This is simply to avoid the need for escaping the backslashes and quotes.

Function `re.findall(pattern, string)` returns a list of all matches of the `pattern` in the `string`. If the pattern includes one or more groups (as does ours), the list will consist of the matches groups - in our case everything between the quotes after a `href`. The second argument to `re.findall` is the `Page` we retrieved. The result is stored to `Hrefs`.

For instance, this is how we find all the pages to which our laboratory home page, `http://www.ailab.si` links:

```prolog
| ?- getHrefs("http://www.ailab.si", X).
X = ['http://www.ailab.si/pageicon.ico', '/common/styles.css', /,
     '/people.htm', '/publications.htm', '/activities.htm', '/projects.htm',
     '/software.htm','/teaching.htm','http://www.fri.uni-lj.si'|...] ?
yes
```

Calling Prolog from Python
--------------------------

To call Sicstus from Python, import module `picstus` which contains the high-level interface described here and, through importing the C++ part of the interface, the low-level structures described in a separate chapter. For simplicity, examples in this section will assume that we have loaded the module with `from picstus import *`. This is not a recommended practice in Python, though. If we load the module by `import picstus`, all the function and class names have to be prefixed by `picstus`.

Examples in this section will use the relations defined in the file `bas.pl`. Among other stuff, it contains three parenthood relations:

```prolog
parent(adam, abel).
parent(eve, abel).
parent(god, god).
```

We can load the file by calling `consult("bas.pl")`

### Getting Solutions One by One

Function `solutions(query_string, **flags)` constructs a query from the given string which produces solutions one at a time. This is especially useful if the query has lots of solutions (or possibly an infinite number of them), since we can simply break out of the loop whenever we find what we look for.

Solutions are given as dictionaries where keys are variable names, and the corresponding values are their matches. Prolog's integers, floats, atoms and lists are converted into Python's integers, floats, strings and lists. Compound terms are given as instances of `Term` (see the chapter on low-level structured). To have all terms stored as (unconverted) instances of `Term`, add `convertToPython=False` as a flag.

```python
>>> for t in solutions("parent(X, Y)"):
...     print t
{'Y': 'abel', 'X': 'adam'}
{'Y': 'abel', 'X': 'eve'}
{'Y': 'god', 'X': 'god'}
```

```python
>>> for t in solutions("parent(X, X)"):
...     print t
{'Y': 'god', 'X': 'god'}
```

```python
>>> for i in solutions("parent(adam, abel)"):
...     print i
{}
```

Underscores can be used as in Prolog: they are omitted from the returned dictionary.

```python
>>> for t in solutions("parent(X, _)"):
...     print t
{'X': 'adam'} {'X': 'eve'}
```

```python
>>> for i in solutions("parent(adam, _)"):
...     print i
{}
```

```python
>>> for i in solutions("parent(_, _)"):
...     print i {}
```

The last two cases were somewhat special: the query has a solution, but the dictionary is empty since there are no variable values to be set. This is similar to one of the examples above, where we asked about `parent(adam, abel)`. On the other hand, the following loop prints nothing, since Abel is not Adam's parent, so there are no solutions.

```python
>>> for i in solutions("parent(abel, adam)"):
...     print i
```

We can also use the function `solutions` to test whether any solutions exist. If there or no solutions, `solutions` returns `False`. If there are solutions and the query contains no named variables, it will return `True`.

```python
>>> print solutions("parent(adam, abel)")
True
>>> print solutions("parent(abel, _)")
False
>>> print solutions("parent(_, _)")
True
```

In the third case - if there are solutions and there are named variables - the `solutions` returns a generator which generates the solutions one by one. When used in an `if` statement, such a generator evaluates as `True`, which is what we need, so the following code would work as intended.

```python
if solutions("parent(X, abel)"):
    print "abel has parents"
else:
    print "abel has no parents"
```

Due to technical reasons, we should not store the result of `solutions` in any non-temporary variable, like this.

```python
b = solutions("parent(X, abel)")
if b:
    print "abel has parents"
else:
    print "abel has no parents"
```

For as long as `b` remains alive, the query remains open. And even worse, when `b` is finally disposed, all the terms and queries created after `b` die with it. This technical issue is explained in the section on the lifetime of queries, but we don't have to read it: just avoid storing the result of `solutions`.

There is also a function `exists` which returns `True` or `False`, telling whether there are any solutions or not. This function, described below, is safer for unexperienced users.

### Getting All Solutions at Once

When the number of solutions is known to be low enough and we want to get them all, call `findall(query_string, **flags)`. If the number of solutions is huge (or infinite), `findall` will freeze (and probably crash).

Here are a few examples.

```python
>>> print findall("parent(X, Y)")
[{'Y': 'abel', 'X': 'adam'}, {'Y': 'abel', 'X': 'eve'},
 {'Y': 'god', 'X': 'god'}]
```

```python
>>> print findall("parent(X, abel)")
[{'X': 'adam'}, {'X': 'eve'}]
```

```python
>>> print findall("parent(_, abel)")
[{}]
```

```python
>>> print findall("parent(abel, _)")
[]
```

It may be instructive to see how `findall` is implemented. Apart from the two parameters (`a_terms` and `kwds`), the function is trivial:

```python
def findall(s, a_terms=None, **kwds):
    return list(solutions(s, a_terms, **kwds))
```

### Checking for Solutions

When we are only interested in whether a solution exists, use the function `exists(query_string)`.

```python
>>> print exists("parent(adam, abel)")
True
```

```python
>>> print exists("parent(abel, adam)")
False
```

```python
>>> print exists("parent(adam, X)")
True
```

```python
>>> print exists("parent(abel, _)")
False
```

The function simply calls `solutions` and converts the result to a boolean:

```python
def exists(s, a_terms=None, **kwds):
    return bool(solutions(s, a_terms, **kwds))
```

### More Control: Passing Terms

There is an alternative way to calling the above functions: instead of `solutions("parent(adam, abel)")` we can replace the query string with the name of the predicate, and then add the list of terms as a second argument. 

```python
solutions("parent", ["adam", "abel"])
```

What do we gain by doing so? First, we will often query for something stored in variables, not for constants. Say we got someone's name in a web form, stored it in a Python's variable `name` and now we'd like to get his/her parents. Query

```python
solutions("parent", ["X", name])
```

is simpler than

```python
solutions("parent(X, %s)" % name)
```

Of course, what we put inside the list don't need to be strings - we can also use integers and floats here, like

```python
solutions("somepredicate", [1, 2])
```

which would tell whether `somepredicate(1, 2)` is true or not.

But most importantly, besides strings, integers and floats, the list can include variables - terms, which we have constructed before.

```python
>>> a = Term(name="X")
>>> for t in solutions("parent", [a, "abel"]):
...     print t
{'X': 'adam'}
{'X': 'eve'}
```

This is equivalent to calling `solutions("parent(X, abel)")` (this latter code actually transforms to the former). The difference is that in the above example we manually constructed the term and can thus reach it. For instance, like this

```python
>>> for t in solutions("parent", [a, "abel"]):
...     print a
adam
eve
```

Remember that `X` is now a term, not a string (although it prints out as one).

Why do we need `name="X"`? Unnamed terms (or terms whose names begin with underscores or are empty), behave exactly like names that (begin with) underscore. With a small differences: the identity of a term is not established by its name, but by the object. This has two consequences:

- We can have two *different* terms with the same name. This of course wouldn't work because one key would override another in the dictionary. Reading terms values directly, as in the second example, would still work.
- If we name a term `_` (or don't give it a name at all) and use it at different places, it would always represent the same term, disregarding the name. So, while `?- parent(_, _)` would match all parent relations, the following would only give the persons who are their own parents.

  ```python
  a = Term()
  for t in solutions("parent", [a, a]):
      print t
  ```

### Alternative Interface: Fake Atoms and Variables

Some users may prefer a more Prolog-like interface and type things like `parent(X, abel)` directly into Python. This is not possible since Python would see `parent`, `X` and `abel` as undefined variables. However, `picstus` has a trick to offer: we can write the query above by prefixing each atom or variable name with `sp`.

We can use it in the same way as solutions, either in a for loop

```python
>>> for i in sp.parent(sp.X, sp.abel):
...     print i
{'X': 'adam'}
{'X': 'eve'}
```

or in an if statement if the query contains no named variables

```python
>>> if sp.parent(sp._, sp.abel):
...     print "abel has parents"
... else:
...     print "abel has no parents"
```

We can mix the fake `sp` terms and atoms with strings, integers, floats and "true" atoms.

```python
>>> for t in sp.parent(sp.Y, "abel"):
...     print t
{'Y': 'adam'}
{'Y': 'eve'}
>>> for t in sp.parent("Y", sp.abel):
...     print Y
adam
eve
>>> Y = Term()
>>> for t in sp.parent(Y, sp.abel):
...     print Y
adam
```

The first case is trivial. The second does exactly the same as the first: `sp.Y` in the first example actually calls a function which constructs `Term(name="Y")`. In this, second example, we can print out the value of `Y` instead of `t`.

The last example contains a trap: if we create an *unnamed* term and store it in Python variable `Y` this is, from picstus perspective, still an unnamed term, so the last predicate is equivalent to `parent(_, abel)`. It will only find a single solution, store it in `Y` and return `True`. To get both solutions, we'd need to create `Y` as `Y = Term(name="Y")`.

Finally, we can store some stuff in ordinary Python variables and use them for querying. This is also the main intention of the interface based on the "fake" predicates.

```python
abel = "abel"
parent = sp.parent
Y = Term(name="Y")
for t in parent(Y, abel):
    print Y
```

Note: If we load the picstus module by `import picstus`, we may still want to add `from picstus import sp` to avoid having to write things like `picstus.sp.parent(picstus.sp._, picstus.sp.abel)` which is rather cumbersome.

To see how this is implemented, just observe the classes `__Faker` and `__PostponedAtom`. The code is so short that any textual explanation would be longer (and it already is).

### Utility Functions

There's only one utility function at the moment.

consult(filename)
:   Consults the file `filename`. (It's not a big deal: it just executes `picstus.Query("consult", [filename]).next_solution()`; more on that below.)

Low-Level Interface Classes
---------------------------

This part of the document describes the low-level interface. It contains some technical details, which everybody except the potential future developers can safely skip.

The bulk of the interface is contained in four Python classes which represent the Sisctus' atom, term, predicate and query.

### Atom

Instances of `Atom` references a Prolog atom. It's a rather poor object: we can construct it from a string and convert it back to string, like below.

```python
a = _picstus.Atom("an_atom")
aa = str(a)
```

Neither the interface nor Sicstus check for the syntactical correctness of the atom - we can start it with capital letters, include illegal characters ... and pay for it later.

The interface code ensures that Prolog's garbage collection does not dispose the atom by calling `SP_register_atom` when constructing an `Atom` and `SP_unregister_atom` when destructing it. In other words, Python keeps its reference count for the instance of `Atom` and when it drops to zero it decreases the reference count in Sicstus.

### Term

Instances of `Term` represent references to terms in Sicstus: integers, floats, atoms, variables and compound terms.

#### Construction

`Term`'s constructor accepts arguments of various types which determine the type of the term.

`Term()`
: Constructs a variable

`Term(<integer>)`
: Constructs an integer term.

`Term(<float>)`
: Constructs a float term.

`Term(<string>`
: Constructs an atom. We're responsible for making sure that the string is a true atom (e.g., doesn't contain illegal characters etc.); the interface doesn't check it for us, nor does Sicstus.

`Term(<atom>)`
: Puts the atom into the term. The argument should be a `picstus.Atom`, described above.

`Term(<list>)`
: Creates a compound term containing the list. The list argument should be a Python list containing other terms, integers, floats, strings or list of terms (the `Term` constructor gets called recursively on the list elements).

`Term(<string>, <int>)`
: Constructs a compound term, where the first argument gives the functor's name and the second the arity. Arguments are initialized to variables.

`Term(expression, list-of-terms)`
: String `expression` should be a Prolog expression, such as "`parent(X, alice).`", and the second argument is a list of variables (given as terms). If the expression contains no variables, an empty list must be given. (If the expression does not end with a period, the constructor will add it before passing the expression to Sicstus.)

  Sicstus will parse the string, construct the corresponding Prolog structure (a tree of functors) and put the terms given in the list at the corresponding leaves. The length of the list should match the number of (different) variables: if the list is shorter, Sicstus will construct new variables, if it's longer, it will ignore the redundant. The order in which the variables are assigned matches their order in the expression.

  ```python
  a, b, c = Term(), Term(), Term()
  tt = Term("(parent(X, john), parent(Y, X), parent(Z, Y)).", [a, b, c])
  ```

  We first constructed three variables; following Python's conventions, we named them with lowercase first characters. Then we constructed a term from an expression containing three variables. Both instances of `X` are assigned `a`, `Y`s are assigned `b` and `Z`s are assigned `c`. Consider the expression a movie, variables are movie characters and the list `[a, b, c]` are the actors in order of appearance.


#### Setting Terms

Say that a term `t` has been initialized as variable, `t = Term()` and is already included in a larger compound term. Now we want to assign it an integer. Writing `t = Term(15)` won't do the job, since it constructs a new term and assigns it to `t` instead of setting the value of the existing term. (`t = 15` is even worse, since it assigns a plain integer to `t`.)

To set the term to a certain value, use the method `assign`.

```python
t.assign(15)
```

Arguments for assign are exactly the same as for the constructor explained above. (Actually, the constructor merely allocates the memory for the term and then forwards the call to `assign`.)

#### Reading Terms

Prolog terms can be cast to Python types using Python's functions `int`, `float`, `str` and `list`.

```python
t = _picstus.Term(15)
pt = int(t)
t = _picstus.Term([1, 2, 3])
pt = list(t)
```

The term must be of the right type for the cast, for instance, an atom cannot be cast into an integer.

Function `list` can be used for retrieving lists and for retrieving arguments of compound terms. List items or arguments can also be retrieved by indexing.

```python
t = _picstus.Term([1, 2, 3])
t1 = t[1]
t = _picstus.Term("parent(john, eve).", [])
t1 = t[1]
```

Arguments are numbered according to Python's indexing rules: the first argument has index 0.

Casting a compound to a term using `str` gives the functor and its arity. To get only the functor (as a string), use `Term`'s method `functor`.

We can take a length of a list (which returns the number of elements) or of a compound term (which returns its arity).

Numeric terms support some basic arithmetic operations - it is possible to add a number to a term, multiply a term with a number, compute a logarithm of a term, or even a sum of a list (using the built-in function `sum`) if it contains nothing but numbers . However, all binary operations require that one of the operands is an ordinary Python number.The result is always a number, not a term.

#### Checking term types

Method `Term.getType()` returns the term's type: `_picstus.VARIABLE` (1), `_picstus.INTEGER` (2), `_picstus.ATOM` (3), `_picstus.FLOAT` (4) or `_picstus.COMPUND` (5).

To check for individual types, use methods `is_variable()`, `is_integer()`, `is_atom()`, `is_float()`, `is_compound()`, `is_list()`, `is_atomic()` and `is_number()`. Besides the basic types, which correspond to the types listed above, `is_list()` returns true if the term is a non-empty list (it is functor that matters, so empty list is an atom), `is_atomic()` returns true for atoms and numbers, and `is_number()` is true for integers and float.

#### Printing out

The `print` statement or conversion to string by `str` function gives the natural presentation of terms - integers and floats are printed as numbers and atoms are printed as strings. Compound terms (including lists) are printed in the Prolog's usual functor/arity format. Variables are represented with underscore; this is essentially correct since the variables do not have any true names, their names are only tracked by the Prolog's interpreter.

Printing out the term in an interactive session, without using the print statement, gives something like `<picstus.Term 15<`, or `<picstus.Term parent/2>`.

#### Comparison

Terms can be compared using the ordinary Python comparison operators, which function the same as the Prolog operators `@<`, `==` and `@>`.

#### Unification

Two terms can be unified, by calling a function `unify` (this is not a `Term`'s method). The function returns `true` if unifications succeeds and `false` if it doesn't.

```python
X = _picstus.Term()
Y = _picstus.Term()
t = _picstus.Term("parent(john, X)", [X])
u = _picstus.Term("parent(Y, eve)", [Y])
print _picstus.unify(t, u)
print X
print Y
```

After this, `X` will equal `eve` and `Y` will equal `john`.

For a more complicated example, this also works as expected:

```python
X, Y, T = _picstus.Term(), _picstus.Term(), _picstus.Term()
t = _picstus.Term("parent(T, X)", [T, X])
u = _picstus.Term("parent(Y, eve)", [Y])
v = _picstus.Term("parent(john, eve)", [])
_picstus.unify(t, u)
_picstus.unify(v, u)
[]
```

#### Lifetime of terms

Terms are garbage collected at the end of a query. There are two types of queries that Picstus has to control. If Python is called from Prolog, this is a query and all the terms constructed during that call are destroyed when the call returns. Second, if Python constructs a query object (see below), all the terms constructed from that point on are destroyed when the query is closed.

This stack-like garbage collection, which is alien to Python, is controlled by Sicstus, to which the term object belongs. There is no nice way around it, nor do we need one, since this handling of terms should come as natural to a Prolog programmer.

If Python code stored references to Prolog terms, which is destroyed by Prolog, the term will be marked in Python as dead. Trying to use a dead term will raise an exception.

### Predicate

Instances of class `Predicate` represent Sicstus predicates. The only method is have is a constructor, which needs to be given the name of the predicate, its arity and, optionally, a module. The name of the predicate and the module can be given as strings or atoms.

The following predicates are the same:

```python
p = _picstus.Predicate("call", 1)
p = _picstus.Predicate("call", 1, "user")
p = _picstus.Predicate(_picstus.Atom("call"), 1, "user")
p = _picstus.Predicate(_picstus.Atom("call"), 1, _picstus.Atom("user"))
```

Predicates can be used in queries.

### Query

Queries function like this: we construct a query from a predicate and a list of terms, some of which may be variables. Then, we call `next_solution` which finds another solution and puts it into arguments. This goes on for as long as we want, or until we call `cut` or `close`, or the query object is destroyed in Python.


#### Construction

Constructor gets up to three arguments: the predicate, a list of terms and, optionally, the module in which the predicate is.

`Query(predicate[, terms])`
:   Here, predicate is an instance of `Predicate`. Terms is a list of terms, some of which may be variables. The length of the list must equal the arity of the predicate, but can be omitted if if the predicate's arity is 0.

`Query(<string>[, terms, module])`
:   The predicate can be given by name. Its arity is inferred from the size of the terms list (0 if there is no list). The name of the module (as a string or an atom) should be given if the module in which the predicate is to be found is not `user`.

`Query(<atom>, terms, module)`
:   Finally, the predicate can be specified as an atom. We use this when we have the atom ready, as it will spare Sicstus from finding it. Arity is again inferred from the terms list. The list (which may be empty) and the module are mandatory.

Here are some examples.

```python
p = _picstus.Query("consult", ["bas.pl"])
```

This is a query which, when run (see below), would consult the file `bas.pl`.

Now follows the classic: the query for extracting all parent-child relations.

```python
X = _picstus.Term()
Y = _picstus.Term()
a = _picstus.Atom("parent")
q = _picstus.Query(a, [X, Y], "user")
```

We have given the predicate as an atom now, so we had to add the module, too. Now the same with a predicate and with the child fixed:

```python
pr = _picstus.Predicate("parent", 2)
X = _picstus.Term()
q = _picstus.Query(pr, [X, "eve"])
```

If we add the module or change the list size in this example, the query
construction will fail.


#### Getting the solutions

Well, this part is trivial: just keep calling `next_solution` until it returns false or we get bored. So, to find Eve's parents from the above example, we say

```python
while q.next_solution():
    print X
```

We can only call `next_solution` for the latest, innermost query. Imagine queries put onto a stack; we can only get the solutions from the latest.



#### Closing the query

Queries can be closed in four ways. First, when `next_solution` finds no (more) solutions, the corresponding query is closed. The query is also closed if it was created in Python code but is no longer referred to by any Python variable. We can explicitly close the query with the following two functions:

`cut()`
: This discards the choices since the query was opened, like the goal `!`. The current solution is retained in the arguments until backtracking into any enclosing query.

`close()`
: This is like `!, fail`: it discard the choices created since the query was opened and then backtracks into the query, throwing away any current solution.

The query closed doesn't need to be the latest, innermost query. Closing any query also closes all the queries which were opened after ("inside") it.

As we have learned in the section about queries, when a query dies, so do all the terms created while the query was active.


#### Lifetime of queries

As mentioned above, if we create a query B inside query A, and the close query A, query B is closed as well, and along with them, all the terms created while the queries were active. Since it is impossible to delete Python variables that may be referencing these queries, the corresponding Python wrappers are marked as dead. Trying to find the solutions of any closed query will raise an exception.

This may be sometimes confusing but there is no other way to couple the stack-based Sicstus' memory management with the heap-based Python's. A particularly nasty and hard-to-discover situations can occur when a certain variable name is reused in Python, like in the following case.

```python
p = _picstus.Query("consult", ["bas.pl"])
p.next_solution()
p = _picstus.Query("parent(john, eve)", [])
p.next_solution()
```

The first line constructs a query and the second gets the first solution (that is, the file is loaded). If we called `next_solution` for one more time, the query would fail and close, but since we have not the query is still open. Now we construct another query by calling `_picstus.Query("parent(john, eve)", [])`. But when we assign it to `p`, the query to which `p` referred before is closed (since no other Python variables refer to it). Along with it, the second query is killed, too. So the final `next_solution` raises an exception.

To avoid the problem, we could call the first query until it fails, or close it manually, delete variable `p` (either with `del p` or by assigning it anything, e.g. `p=42`) or use another variable name for the second query.
