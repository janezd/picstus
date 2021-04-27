#ifndef __CLASSES_HPP
#define __CLASSES_HPP

#include "Python.h"


class TString {
    char *start, *last, *end;

public:
    TString();
    ~TString();
    bool putc(char c);
    bool puts(char *s);
    char *releaseString();
    const char *getString()
    { return start; }
};    





#define PYERROR(tpe,msg,errcode) { PyErr_Format(tpe, msg); return errcode; }
#define RETURN_NONE { Py_INCREF(Py_None); return Py_None; }

#define ASSERT_err(cond,err,errcode) if (!(cond)) PYERROR(PyExc_TypeError, err, errcode)
#define ASSERT(cond,err) ASSERT_err(cond,err,NULL)
#define ASSERT_1(cond,err) ASSERT_err(cond,err,-1)
#define ASSERT_false(cond,err) ASSERT_err(cond,err,false)

extern bool pythonInitialized;

class TPyTerm {
public:
  PyObject_HEAD
  SP_term_ref term_ref;

  bool alive;
  TPyTerm **prev, *next;

  PyObject *attrdict;
};

extern PyTypeObject *PyTerm_Type;
#define PyTerm_AsTerm(x) (((TPyTerm *)(x))->term_ref)
int cc_Term(PyObject *, void *);
PyObject *PyTerm_New(SP_term_ref);
PyObject *term_new(PyTypeObject *type, PyObject *args, PyObject *kwds);
inline bool PyTerm_Check(PyObject *obj) { return obj->ob_type == PyTerm_Type; }

#define CHECK_TERM_ALIVE(term,err) { if (!((TPyTerm *)(term))->alive) PYERROR(PyExc_SystemError, "term is dead (it belonged to an ended query)", err); }

PyObject *term_toPython_low(const SP_term_ref &tref);
PyObject *term_toPython(PyObject *);
bool term_from_python(PyObject *arg, SP_term_ref &tref);
bool term_assign_low(SP_term_ref &term, PyObject *args);
PyObject *list_to_python(SP_term_ref list, bool tuple);
char *list_to_chars(SP_term_ref list);
SP_term_ref *list_to_array(SP_term_ref list, int &size);
char *stringFromTerm(SP_term_ref term);
bool termIsList(SP_term_ref term);



class TPyAtom {
public:
  PyObject_HEAD

  SP_atom atom;

  PyObject *attrdict;
};

extern PyTypeObject *PyAtom_Type;
#define PyAtom_AsAtom(x) (((TPyAtom *)(x))->atom)
int cc_Atom(PyObject *, void *);
PyObject *PyAtom_New(SP_atom);
inline bool PyAtom_Check(PyObject *obj) { return obj->ob_type == PyAtom_Type; }



class TPyPredicate {
public:
  PyObject_HEAD
  SP_pred_ref predicate;

  char *name;
  long arity;

  PyObject *attrdict;
};

extern PyTypeObject *PyPredicate_Type;
#define PyPredicate_AsPredicate(x) (((TPyPredicate *)(x))->predicate)
int cc_Predicate(PyObject *, void *);
inline bool PyPredicate_Check(PyObject *obj) { return obj->ob_type == PyPredicate_Type; }



class TPyQuery {
public:
  PyObject_HEAD
  SP_qid qid;

  TPyQuery *previousQuery;

  bool alive;
  TPyTerm *terms, **lastTerm;

  PyObject *attrdict;
};

extern TPyQuery *queryStack;


#define CHECK_QUERY_ALIVE(query,err) { if (!((TPyQuery *)(query))->alive) PYERROR(PyExc_SystemError, "query is closed", err); }

extern PyTypeObject *PyQuery_Type;
#define PyQuery_AsQuery(x) (((TPyQuery *)(x))->qid)
int cc_Query(PyObject *, void *);
inline bool PyQuery_Check(PyObject *obj) { return obj->ob_type == PyQuery_Type; }


extern PyObject *picstusModule;
extern PyObject *PyExc_SicstusError;

void readyModule();
bool isPrologType(PyObject *);

PyObject *unify(PyObject *t1, PyObject *t2);


int getListLength(SP_term_ref list);

#endif