:- module(python, []).

foreign_resource('python', [python_exec, python_set, python_eval, python_call, init(init), deinit(deinit)]).

foreign(python_exec, exec(+term)).
foreign(python_eval, eval(+term, -term)).
foreign(python_set, set(+term, +term)).
foreign(python_call, call(+term, +term, -term)).

:- load_foreign_resource('python').

execfile(F) :-
   call(execfile, [F], _).


% picstus will change %% to % and insert the value of M in place of %s.

import(M) :-
   exec("exec('import %%s' %% '%s')" < M).

importfrom(M) :-
   exec("exec('from %%s import *' %% '%s')" < M).
   