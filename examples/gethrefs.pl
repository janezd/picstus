:- consult(python).

:- python:import(re).
:- python:import(urllib).

getHrefs(URL, Hrefs) :-
    python:eval("urllib.urlopen('%s').read()" < [URL], Page),
    python:call("re.findall", ['(?i)href\s*=\s*"([^"]*)"', Page], Hrefs).
