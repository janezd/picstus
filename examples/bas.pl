parent(adam, abel).
parent(eve, abel).
parent(god, god).

male(adam).
female(eve).
male(abel).
male(god).
female(god).

grandparent(X, Y) :-
  parent(X, Z), parent(Z, Y).


:- op(600, xfx, --->).
adam ---> abel.

