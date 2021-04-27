/*

To compile picstus, locate spaux.c somewhere in Sicstus directories, find function
sp_main_helper and eliminate checking for multiple use of the resource. That is,
remove everything between 

  (void)api;

and

  params->funcs = funcs;

(at least in my version of the file).

There must be another way to do this, but I'm too lazy to find it. Suggestions welcome.

*/


extern "C" {
#include <sicstus/sicstus.h>
#if SPDLL
#include "python_glue.h"
#endif
}

#include "Python.h"


#include "picstus.hpp"

bool pythonInitialized = false;

PyObject *PyExc_SicstusError = NULL;


TString::TString()
{ 
  start = last = new char[256];
  *last = 0;
  end = start + 256;
}


TString::~TString()
{ 
  if (start)
    delete start;
}


bool TString::putc(char c)
{
  if (last == end-1) {
    const int size = end - start;
    const int nsize = size < 10240 ? size << 1 : size + 2048;
    char *nstart = (char *)realloc(start, nsize);
    if (!nstart)
      return false;
    start = nstart;
    last = start + size - 1;
    end = start + nsize;
  }

  *last = c;
  *++last = 0;

  return true;
}


bool TString::puts(char *s)
{
  while(*s)
    if (!putc(*s++))
      return false;
  return true;
}


char *TString::releaseString()
{ 
  char *res = start;
  start = last = new char[256];
  end = start + 256;
  *last = 0;
  return res;
}



class TStringBuffer : public TString {
public:
    long stream_code;
    SP_stream *stream;

    TStringBuffer()
    : TString(),
      stream_code(0),
      stream(NULL)
    { 
      SP_make_stream(this, NULL, sb_putc, NULL, NULL, NULL, close, &stream);
      SP_term_ref tmp = SP_new_term_ref();
      SP_put_address(tmp, stream);
      SP_get_integer(tmp, &stream_code);
    }

    ~TStringBuffer()
    {
      SP_fclose(stream);
    }

    static int sb_putc(char c, void *handle)
    {
      ((TStringBuffer *)handle)->putc(c);
      return c;
    }

    static int close(void *handle)
    {
      return 0;
    }
};



void setSicstusException()
{
  TStringBuffer buf;

  SP_term_ref goal = SP_new_term_ref();
  SP_term_ref stream = SP_new_term_ref();
  SP_term_ref streamCode = SP_new_term_ref();
  SP_term_ref term = SP_new_term_ref();

  SP_term_ref vals[4]; /*  = {stream, streamCode, term, 0}; */
  vals[0]=stream;
  vals[1]=streamCode;
  vals[2]=term;
  vals[3]=0;

  SP_put_variable(stream);
  SP_put_integer(streamCode, buf.stream_code);
  SP_exception_term(term);
  SP_read_from_string(goal, "prolog:stream_code(Stream, StreamCode), prolog:write(Stream, Term).", vals);
  
  SP_pred_ref call_pred = SP_predicate("call", 1, "prolog");
  SP_query_cut_fail(call_pred, goal);

  PyErr_Format(PyExc_SicstusError, buf.getString());
}


////////////////////////////////////////////////////////////////////////
//**************    Atom

SP_atom emptyListAtom;
SP_atom noneAtom;


void assignDictFromDict(PyObject *self, PyObject *dict)
{
  if (!dict)
    return;

  PyObject *key, *value;
  int pos = 0;

  while (PyDict_Next(dict, &pos, &key, &value))
    PyObject_SetAttr(self, key, value);
}


void atom_dealloc(PyObject *self)
{ 
  SP_unregister_atom(PyAtom_AsAtom(self));
  PyObject_Del(self);
}

PyObject *atom_str(PyObject *self)
{
  return PyString_FromString(SP_string_from_atom(PyAtom_AsAtom(self)));
}

PyObject *atom_repr(PyObject *self)
{
  char *sa = SP_string_from_atom(PyAtom_AsAtom(self));
  char *ts = new char[strlen(sa) + 20]; 
  sprintf(ts,"<picstus.Atom '%s'>", sa);
  PyObject *res = PyString_FromString(ts);
  delete ts;
  return res;
}

int atom_length(PyObject *self)
{
  return SP_atom_length(PyAtom_AsAtom(self));
}

PyMethodDef atom_methods[] = {
     {NULL, NULL}
};


PyObject *atom_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  PyObject *self = NULL;

  if (args && (PyTuple_Size(args) == 1)) {
    PyObject *arg = PyTuple_GET_ITEM(args, 0);
    if (PyString_Check(arg)) { 
      self = type->tp_alloc(type, 0);
      PyAtom_AsAtom(self) = SP_atom_from_string(PyString_AsString(arg));
    }

    else if (PyTerm_Check(arg)) {
      SP_term_ref &term = PyTerm_AsTerm(arg);
      ASSERT(SP_is_atom(term), "the term is not an atom");

      self = type->tp_alloc(type, 0);
      SP_get_atom(PyTerm_AsTerm(arg), &PyAtom_AsAtom(self));
    }

    else
      PYERROR(PyExc_TypeError, "Atom expects a string or term (which is an atom) as an argument", NULL);
  }

  if (!self || !PyAtom_AsAtom(self) || !SP_register_atom(PyAtom_AsAtom(self))) {
    Py_DECREF(self);
    PYERROR(PyExc_TypeError, "invalid arguments", NULL);
  }

  ((TPyAtom *)self)->attrdict = NULL;
  assignDictFromDict(self, kwds);

  return self;
}


PySequenceMethods atom_as_sequence = {
  atom_length,                      /* sq_length */
  0, 0,
  0,               /* sq_item */
  0,              /* sq_slice */
  0,            /* sq_ass_item */
  0,           /* sq_ass_slice */
  0,
};


PyTypeObject PyAtom_Type_s = {
  PyObject_HEAD_INIT((_typeobject *)&PyType_Type)
  0,
  "pisctus.Atom",
  sizeof(TPyAtom), 0,
  atom_dealloc,
  0, 
  0, 0, 0, atom_repr,
  0, &atom_as_sequence, 0, /* protocols */
  0, 0,
  atom_str, 0, 0, 0,
  Py_TPFLAGS_DEFAULT, /* tp_flags */
  "",                                              /* tp_doc */
  0, 0, 0, 0, 0, 0, 
  atom_methods,
  0, 0, 0, 0, 0, 0, offsetof(TPyAtom, attrdict), 0,
  PyType_GenericAlloc, atom_new, 0, 0, 0, 0, 0, 0, 0,
};


PyTypeObject *PyAtom_Type = &PyAtom_Type_s;

PyObject *PyAtom_FromAtom(SP_atom atom)
{
  TPyAtom *pyatom = PyObject_New(TPyAtom, PyAtom_Type);

  if (pyatom) {
    Py_INCREF((PyObject *)pyatom);
    pyatom->atom = atom;
    SP_register_atom(atom);
  }

  return (PyObject *)pyatom;
}


PyObject *PyAtom_FromString(char *atom)
{
  TPyAtom *pyatom = PyObject_New(TPyAtom, PyAtom_Type);

  if (pyatom) {
    Py_INCREF((PyObject *)pyatom);
    pyatom->atom = SP_atom_from_string(atom);
    SP_register_atom(pyatom->atom);
  }

  return (PyObject *)pyatom;
}


int cc_Atom(PyObject *obj, void *ptr)
{ if (obj->ob_type != PyAtom_Type)
    return 0;
  *(SP_atom *)ptr = PyAtom_AsAtom(obj);
  return 1;
}



////////////////////////////////////////////////////////////////////////
//**************    PyTerm



PyObject *term_alloc(PyTypeObject *type, int size)
{
  PyObject *self = PyType_GenericAlloc(type, size);

  TPyTerm *pyterm = (TPyTerm *)(self);

  if (queryStack) {
    pyterm->prev = queryStack->lastTerm;
    pyterm->next = NULL;
    *(queryStack->lastTerm) = pyterm;
    queryStack->lastTerm = &pyterm->next;
  }
  else {
    pyterm->prev = NULL;
    pyterm->next = NULL;
  }

  pyterm->alive = true;

  return self;
}



/* We don't mind if the term is still alive in Prolog;
   we only delete the Python object, the term may live on.

   But we have to extract the term from the list of terms. */

void term_dealloc(PyObject *self)
{ 
  TPyTerm *pyterm = (TPyTerm *)self;

  if (pyterm->alive && pyterm->prev) {
    *pyterm->prev = pyterm->next;
    if (pyterm->next)
      pyterm->next->prev = pyterm->prev;
  }

  PyObject_Del(self);
}



char *term_as_string(PyObject *self)
{
  SP_term_ref &term = PyTerm_AsTerm(self);

  switch (SP_term_type(term)) {
    case SP_TYPE_ATOM: {
      char *s;
      SP_get_string(term, &s);
      return strcpy(new char[strlen(s)+1], s);
    }
  
    case SP_TYPE_INTEGER: {
      long i;
      SP_get_integer(term, &i);
      char *res = new char[20];
      snprintf(res, 19, "%i", i);
      return res;
    }

    case SP_TYPE_FLOAT: {
      double d;
      SP_get_float(term, &d);
      PyObject *pyf = PyFloat_FromDouble(d);
      PyObject *fs = PyObject_Str(pyf);
      char *res = PyString_AsString(fs);
      res = strcpy(new char[strlen(res)+1], res);
      Py_DECREF(pyf);
      Py_DECREF(fs);
      return res;
    }

    case SP_TYPE_COMPOUND: {
      SP_atom atom;
      int arity;
      SP_get_functor(term, &atom, &arity);
      char *satom = SP_string_from_atom(atom);
      char *res = new char[strlen(satom)+15];
      sprintf(res, "%s/%i", satom, arity);
      return res;
    }

    case SP_TYPE_VARIABLE: {
      return strcpy(new char[2], "_");
    }
  }

  return NULL;
}


PyObject *term_str(PyObject *self)
{
  CHECK_TERM_ALIVE(self, NULL);

  char *s = term_as_string(self);
  if (!s)
    return PyString_FromString("<picstus.Term>");

  PyObject *r = PyString_FromString(s);
  delete s;
  return r;
}


PyObject *term_repr(PyObject *self)
{
  CHECK_TERM_ALIVE(self, NULL);

  char *s = term_as_string(self);
  if (!s)
    return PyString_FromString("<picstus.Term>");

  char *ts = new char[strlen(s) + 20]; 
  sprintf(ts, SP_term_type(PyTerm_AsTerm(self)) ==  SP_TYPE_ATOM ? "<picstus.Term '%s'>" : "<picstus.Term %s>", s);
  delete s;
  PyObject *res = PyString_FromString(ts);
  delete ts;
  return res;
}


PyObject *term_int(PyObject *self)
{
  CHECK_TERM_ALIVE(self, NULL);

  SP_term_ref &term = PyTerm_AsTerm(self);

  if (SP_is_float(term)) {
    double i;
    ASSERT(SP_get_float(term, &i), "cannot convert the term to integer");
    return PyInt_FromLong(long(i));
  }

  long i;
  ASSERT(SP_is_integer(term), "term is not an integer");
  ASSERT(SP_get_integer(term, &i), "cannot convert the term to integer");
  return PyInt_FromLong(i);
}


PyObject *term_float(PyObject *self)
{
  CHECK_TERM_ALIVE(self, NULL);

  SP_term_ref &term = PyTerm_AsTerm(self);
  if (SP_is_integer(term)) {
    long i;
    ASSERT(SP_get_integer(term, &i), "cannot convert the term to float");
    return PyFloat_FromDouble(i);
  }

  double i;
  ASSERT(SP_is_float(term), "term is not a float");
  ASSERT(SP_get_float(term, &i), "cannot convert the term to float");
  return PyFloat_FromDouble(i);
}


int term_coerce(PyObject **self, PyObject **other)
{ 
  CHECK_TERM_ALIVE(*self, NULL);

  SP_term_ref &term = PyTerm_AsTerm(*self);

  if (PyInt_Check(*other)) {
    // Although term_int can handle floats, we shouldn't call it:
    //  if the term is float, *other should be converted to float, too
    if (SP_is_integer(term)) {
      *self = term_int(*self);
      Py_INCREF(*other);
      return 0;
    }

    if (SP_is_float(term)) {
      *self = term_float(*self);
      long l = PyInt_AsLong(*other);
      *other = PyFloat_FromDouble(l);
      return 0;
    }
  }


  if (PyFloat_Check(*other)) {
    PyObject *s2 = term_float(*self);
    if (s2) {
      *self = s2;
      Py_INCREF(*other);
      return 0;
    }

    PyErr_Clear();
  }

  return 0;
}


PyObject *term_getattr(PyObject *self, char *s)
{
  CHECK_TERM_ALIVE(self, NULL);

  SP_term_ref &term = PyTerm_AsTerm(self);

  #define CHECK(what) if (!strcmp(s, "is_"#what)) return PyBool_FromLong(SP_is_##what(term));
  #undef CHECK


  PyObject *pyname = PyString_FromString(s);
  PyObject *res = PyObject_GenericGetAttr((PyObject *)self, pyname);
  Py_DECREF(pyname);
  return res;
}


PyObject *term_getFunctor(PyObject *self)
{
  CHECK_TERM_ALIVE(self, NULL);

  SP_atom atom;
  int arity;
  ASSERT(SP_get_functor(PyTerm_AsTerm(self), &atom, &arity), "cannot read functor");
  return PyString_FromString(SP_string_from_atom(atom));
}


PyObject *term_getArity(PyObject *self)
{
  CHECK_TERM_ALIVE(self, NULL);

  SP_atom atom;
  int arity;
  ASSERT(SP_get_functor(PyTerm_AsTerm(self), &atom, &arity), "cannot read functor");
  return PyInt_FromLong(arity);
}


PyObject *term_getType(PyObject *self)
{
  CHECK_TERM_ALIVE(self, NULL);
  return PyInt_FromLong(SP_term_type(PyTerm_AsTerm(self)));
}

#define TERM_IS(what) PyObject *term_is##what(PyObject *self) { CHECK_TERM_ALIVE(self, NULL); return PyBool_FromLong(SP_is_##what(PyTerm_AsTerm(self))); }
  TERM_IS(variable);
  TERM_IS(integer);
  TERM_IS(float);
  TERM_IS(atom);
  TERM_IS(compound);
  TERM_IS(list);
  TERM_IS(atomic);
  TERM_IS(number);
#undef TERM_IS


SP_term_ref *termArray_from_python(PyObject *termlist)
{
  if (!termlist) {
    SP_term_ref *term_refs = new SP_term_ref[1];
    *term_refs = NULL;
    return term_refs;
  }

  const long arity = PyList_Size(termlist);
  SP_term_ref *term_refs = new SP_term_ref[arity+1], *tr = term_refs;
  for(int i = 0; i < arity; i++, tr++) {
    PyObject *li = PyList_GET_ITEM(termlist, i);
    if (PyTerm_Check(li))
      *tr = PyTerm_AsTerm(li);
    else {
      *tr = SP_new_term_ref();
      if (!term_assign_low(*tr, li)) {
        delete term_refs;
        return NULL;
      }
    }
  }
  *tr = NULL;

  return term_refs;
}


bool list_from_python(SP_term_ref &tail, PyObject *list, bool makenewref = true)
{
  const bool isTuple = PyTuple_Check(list);
  ASSERT_err(isTuple || PyList_Check(list), "a list or tuple expected", false);

  SP_term_ref head, oldtail;

  if (makenewref)
    tail = SP_new_term_ref();
  SP_put_atom(tail, emptyListAtom);

  for(int lindex = PyObject_Size(list); lindex--; ) {
    head = SP_new_term_ref();
    if (!term_assign_low(head, isTuple ? PyTuple_GetItem(list, lindex) : PyList_GetItem(list, lindex)))
      return false;

    oldtail = tail;
    SP_cons_list(tail, head, oldtail);
  }

  return true;
}


/* Left out: SP_put_list_chars
             SP_put_list_n_chars
             SP_put_variable, SP_put_list (separate methods)
             SP_put_address  (no use for it)
             SP_put_integer_bytes (we use SP_put_number_chars - easier to create a LongObject in Python),
             SP_cons_functor (can't call it)
*/

bool term_assign_low(SP_term_ref &term, PyObject *args)
{
  if (!args || args == Py_None || PyTuple_Check(args) && !PyTuple_Size(args)) {
    SP_put_variable(term);
    return true;
  }


  if (!PyTuple_Check(args) || PyTuple_Size(args) == 1) {
    PyObject *obj = PyTuple_Check(args) ? PyTuple_GET_ITEM(args, 0) : args;

    if (   (obj==Py_None) && SP_put_variable(term)
        || PyInt_Check(obj) && SP_put_integer(term, PyInt_AsLong(obj))
        || PyFloat_Check(obj) && SP_put_float(term, PyFloat_AsDouble(obj))
        || PyString_Check(obj) && SP_put_string(term, PyString_AsString(obj))
        || PyAtom_Check(obj) && SP_put_atom(term, PyAtom_AsAtom(obj))
        || PyTerm_Check(obj) && SP_put_term(term, PyTerm_AsTerm(obj))
       )
      return true;

    if (PyLong_Check(obj)) {
      obj = PyObject_Str(obj);
      bool res = SP_put_number_chars(term, PyString_AsString(obj)) != 0;
      Py_DECREF(obj);
      ASSERT_false(res, "cannot convert the term to a number");
      return true;
    }

    if (PyList_Check(obj)) {
      SP_term_ref newlist;
      if (list_from_python(newlist, obj)) {
        SP_put_term(term, newlist);
        return true;
      }
    }

    PyErr_Format(PyExc_TypeError, "cannot construct a Term from %s", obj->ob_type->tp_name);
    return false;
  }


  if (PyTuple_Size(args) == 2) {
    char *expr;
    PyObject *arg2;
    if (PyArg_ParseTuple(args, "sO:term.assign", &expr, &arg2)) {

      if (PyList_Check(arg2)) {
        SP_term_ref *term_refs = termArray_from_python(arg2);
        if (!term_refs)
          return false;

        // if the dot is missing, add it
        int exprlen = strlen(expr);
        char *e2 = NULL;
        if (expr[exprlen-1] != '.') {
          e2 = strcpy(new char[exprlen+2], expr);
          e2[exprlen] = '.';
          e2[exprlen+1] = 0;
        }

        int res = SP_read_from_string(term, e2 ? e2 : expr, term_refs);
        delete term_refs;
        if (e2)
          delete e2;

        if (res == SP_ERROR)
          PYERROR(PyExc_SystemError, "error in expression", false);

        return true;
      }


      if (PyInt_Check(arg2)) {
        SP_atom atom = SP_atom_from_string(expr);
        ASSERT_false(SP_put_functor(term, atom, PyInt_AsLong(arg2)), "cannot set term to functor");
        return true;
      }


      PYERROR(PyExc_SystemError, "the second argument to Term() must be a list of terms or arity of a functor", false);
    }

    PYERROR(PyExc_TypeError, "the first argument to Term(s, O) must be a string", false);
  }

  PYERROR(PyExc_TypeError, "term.assign expects 0-2 arguments", false);
}


PyObject *term_assign(PyObject *self, PyObject *args)
{
  CHECK_TERM_ALIVE(self, NULL);

  if (!term_assign_low(PyTerm_AsTerm(self), args))
    return NULL;

  RETURN_NONE
}


bool termIsList(SP_term_ref term)
{
  char *rema;
  return (SP_is_list(term) || SP_is_atom(term) && SP_get_string(term, &rema) && !strcmp(rema, "[]"));
}


int getListLength(SP_term_ref list)
{
  int length = 0;
  SP_term_ref head = SP_new_term_ref(),
              tail = SP_new_term_ref(),
              alist = SP_new_term_ref();

  for(SP_put_term(alist, list); SP_is_list(alist); SP_put_term(alist, tail), length++)
    SP_get_list(alist, head, tail);

  return length;
}


SP_term_ref *list_to_array(SP_term_ref list, int &size)
{
  size = getListLength(list);

  SP_term_ref *res = new SP_term_ref[size];
  SP_term_ref *resi = res;

  SP_term_ref head = SP_new_term_ref(),
              tail = SP_new_term_ref(),
              alist = SP_new_term_ref();

  for(SP_put_term(alist, list); SP_is_list(alist); SP_put_term(alist, tail)) {
    SP_get_list(alist, head, tail);
    *resi = SP_new_term_ref();
    SP_put_term(*resi++, head);
  }

  char *rema;
  if (!SP_is_atom(alist) || !SP_get_string(alist, &rema) || strcmp(rema, "[]")) {
    delete res;
    return NULL;
  }

  return res;
}


PyObject *list_to_python(SP_term_ref list, bool tuple)
{
  int size;
  SP_term_ref *listArray = list_to_array(list, size);
  if (!listArray)
    PYERROR(PyExc_TypeError, "invalid list", NULL);
    
  PyObject *res = tuple ? PyTuple_New(size) : PyList_New(size);
  int noel = 0;
  for(SP_term_ref *ai = listArray, *ae = ai+size; ai != ae; ai++) {
    PyObject *newel = term_toPython_low(*ai);
    if (!newel) {
      Py_DECREF(res);
      res = NULL;
      break;
    }

    if (tuple)
      PyTuple_SetItem(res, noel++, newel);
    else
      PyList_SetItem(res, noel++, newel);
  }

  delete listArray;
  return res;
}



char *list_to_chars(SP_term_ref list)
{
  int length = getListLength(list);
  char *res = new char[length+1], *ri = res;

  for(SP_term_ref head = SP_new_term_ref(), tail = SP_new_term_ref(); SP_is_list(list); SP_put_term(list, tail)) {
    SP_get_list(list, head, tail);
    if (!SP_is_integer(head)) {
      delete res;
      PYERROR(PyExc_TypeError, "list of integers expected", NULL);
    }

    long l;
    SP_get_integer(head, &l);
    *ri++ = (char)l;
  }

  *ri = 0;

  char *rema;
  if (!SP_is_atom(list) || !SP_get_string(list, &rema) || strcmp(rema, "[]")) {
    delete res;
    PYERROR(PyExc_TypeError, "invalid list", NULL);
  }

  return res;
}


PyObject *term_toPython_low(const SP_term_ref &tref)
{
  switch (SP_term_type(tref)) {
    case SP_TYPE_INTEGER: {
      long l;
      if (SP_get_integer(tref, &l) == SP_ERROR)
        PYERROR(PyExc_SystemError, "error converting term into integer", NULL);
      return PyInt_FromLong(l);
    }

    case SP_TYPE_FLOAT: {
      double f;
      if (SP_get_float(tref, &f) == SP_ERROR)
        PYERROR(PyExc_SystemError, "error converting term into float", NULL);
      return PyFloat_FromDouble(f);
    }

    case SP_TYPE_ATOM: {
      char *s;
      SP_get_string(tref, &s);
      if (!s)
        PYERROR(PyExc_SystemError, "error converting term into string", NULL);
      
      if (!strcmp(s, "[]"))
        return PyList_New(0);

      if (!strcmp(s, "none")) {
        Py_INCREF(Py_None);
        return Py_None;
      }

      return PyString_FromString(s);
    }

    case SP_TYPE_COMPOUND: {
      if (SP_is_list(tref))
        return list_to_python(tref, false);

      SP_atom functor;
      int arity;
      SP_get_functor(tref, &functor, &arity);
      char *sfunctor = SP_string_from_atom(functor);

      if (arity == 1) {
        if (!strcmp(sfunctor, "term")) {
          SP_term_ref arg = SP_new_term_ref();
          SP_get_arg(1, tref, arg);
          return PyTerm_New(arg);
        }

        if (!strcmp(sfunctor, "string")) {
          SP_term_ref arg = SP_new_term_ref();
          SP_get_arg(1, tref, arg);

          if (SP_term_type(arg) == SP_TYPE_ATOM) {
            char *s;
            SP_get_string(arg, &s);
            if (!s)
              PYERROR(PyExc_SystemError, "error converting term into string", NULL);
            return PyString_FromString(s);
          }

          else {
            char *s = list_to_chars(arg);
            if (!s)
              return NULL;
            PyObject *res = PyString_FromString(s);
            delete s;
            return res;
          }
        }
      }

      
      break;
    }
  }

  return PyTerm_New(tref);
}


PyObject *term_toPython(PyObject *self)
{
  return term_toPython_low(PyTerm_AsTerm(self));
}


char *stringFromTerm(SP_term_ref term)
{
  if (SP_is_atom(term)) {
    char *s;
    if (SP_get_string(term, &s))
      return strcpy(new char[strlen(s)+1], s);
  }
  
  if (SP_is_list(term))
    return list_to_chars(term);

  PYERROR(PyExc_SystemError, "cannot convert the term to a string", NULL);
}
 
int term_len(PyObject *self)
{
  CHECK_TERM_ALIVE(self, -1);

  SP_term_ref &term = PyTerm_AsTerm(self);

  if (SP_is_atom(term)) {
    char *s;
    SP_get_string(term, &s);
    return strcmp("[]", s) ? strlen(s) : 0;
  }

  if (SP_is_list(term)) {
    int len = 1;
    SP_term_ref head = SP_new_term_ref();
    SP_term_ref tail = SP_new_term_ref();
    SP_get_list(term, head, tail);
    
    while(SP_is_list(tail)) {
      SP_get_list(tail, head, tail);
      len++;
    }

    return len;
  }

  if (SP_is_compound(term)) {
    SP_atom atom;
    int arity;
    SP_get_functor(term, &atom, &arity);
    return arity;
  }

  PYERROR(PyExc_TypeError, "len() of unsized object", -1);
}


PyObject *term_getitem(PyObject *self, int i)
{
  CHECK_TERM_ALIVE(self, NULL);

  SP_term_ref &term = PyTerm_AsTerm(self);

  if (SP_is_atom(term) && !strcmp("[]", SP_string_from_atom(term)))
    PYERROR(PyExc_IndexError, "index out of range (empty list)", NULL);

  if (SP_is_list(term)) {
    SP_term_ref head = SP_new_term_ref();
    SP_term_ref tail = SP_new_term_ref();
    SP_get_list(term, head, tail);
    
    for(; i && SP_is_list(tail); i--)
      SP_get_list(tail, head, tail);

    if (i)
      PYERROR(PyExc_IndexError, "index out of range", NULL);

    return PyTerm_New(head);
  }

  if (SP_is_compound(term)) {
    SP_atom atom;
    int arity;
    SP_get_functor(term, &atom, &arity);
    if (i >= arity)
      PYERROR(PyExc_IndexError, "index out of range", NULL);

    SP_term_ref arg = SP_new_term_ref();
    SP_get_arg(i+1, term, arg);
    return PyTerm_New(arg);
  }


  PYERROR(PyExc_SystemError, "list or functor expected", NULL);
}


int term_compare(PyObject *self, PyObject *other)
{
  CHECK_TERM_ALIVE(self, 0);

  ASSERT(PyTerm_Check(self) && PyTerm_Check(other), "two terms expected");
  return SP_compare(PyTerm_AsTerm(self), PyTerm_AsTerm(other));
}
  

PyObject *term_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  PyObject *self = type->tp_alloc(type, 0);
  PyTerm_AsTerm(self) = SP_new_term_ref();
    
  if (!term_assign_low(PyTerm_AsTerm(self), args)) {
    Py_DECREF(self);
    return NULL;
  }

  ((TPyTerm *)self)->attrdict = NULL;
  assignDictFromDict(self, kwds);

  return self;
}


int cc_Term(PyObject *obj, void *ptr)
{ if (obj->ob_type != PyTerm_Type)
    return 0;
  *(SP_term_ref *)ptr = PyTerm_AsTerm(obj);
  return 1;
}


#define TERM_IS(what) {"is_"#what, (binaryfunc)term_is##what, METH_NOARGS, "() -> bool"}

PyMethodDef term_methods[] = {
     {"assign", (binaryfunc)term_assign, METH_VARARGS, "() -> None"},
     {"getType", (binaryfunc)term_getType, METH_NOARGS, "() -> int"},
     {"toPython", (binaryfunc)term_toPython, METH_NOARGS, "() -> int | float | string | Term"},
     {"functor", (binaryfunc)term_getFunctor, METH_NOARGS, "() -> string"},
     {"arity", (binaryfunc)term_getArity, METH_NOARGS, "() -> int"},
     TERM_IS(variable),
     TERM_IS(integer),
     TERM_IS(float),
     TERM_IS(atom),
     TERM_IS(compound),
     TERM_IS(list),
     TERM_IS(atomic),
     TERM_IS(number),
     {NULL, NULL}
};


PyNumberMethods term_as_number = {
	0, //binaryfunc nb_add;
	0, //binaryfunc nb_subtract;
	0, //binaryfunc nb_multiply;
	0, //binaryfunc nb_divide;
	0, //binaryfunc nb_remainder;
	0, //binaryfunc nb_divmod;
	0, //ternaryfunc nb_power;
	0, //unaryfunc nb_negative;
	0, //unaryfunc nb_positive;
	0, //unaryfunc nb_absolute;
	0, //inquiry nb_nonzero;
	0, //unaryfunc nb_invert;
	0, //binaryfunc nb_lshift;
	0, //binaryfunc nb_rshift;
	0, //binaryfunc nb_and;
	0, //binaryfunc nb_xor;
	0, //binaryfunc nb_or;
	term_coerce, //coercion nb_coerce;
	term_int, //unaryfunc nb_int;
	0, //unaryfunc nb_long;
	term_float, //unaryfunc nb_float;
	0, //unaryfunc nb_oct;
	0, //unaryfunc nb_hex;

	///* Added in release 2.0 */
	0, //binaryfunc nb_inplace_add;
	0, //binaryfunc nb_inplace_subtract;
	0, //binaryfunc nb_inplace_multiply;
	0, //binaryfunc nb_inplace_divide;
	0, //binaryfunc nb_inplace_remainder;
	0, //ternaryfunc nb_inplace_power;
	0, //binaryfunc nb_inplace_lshift;
	0, //binaryfunc nb_inplace_rshift;
	0, //binaryfunc nb_inplace_and;
	0, //binaryfunc nb_inplace_xor;
	0, //binaryfunc nb_inplace_or;

	/* Added in release 2.2 */
	/* The following require the Py_TPFLAGS_HAVE_CLASS flag */
	0, //binaryfunc nb_floor_divide;
	0, //binaryfunc nb_true_divide;
	0, //binaryfunc nb_inplace_floor_divide;
	0 //binaryfunc nb_inplace_true_divide;
};


PySequenceMethods term_as_sequence = {
  term_len,                      /* sq_length */
  0, 0,
  term_getitem,               /* sq_item */
  0,              /* sq_slice */
  0,            /* sq_ass_item */
  0,           /* sq_ass_slice */
  0,
};


PyTypeObject PyTerm_Type_s = {
  PyObject_HEAD_INIT((_typeobject *)&PyType_Type)
  0,
  "pisctus.Term",
  sizeof(TPyTerm), 0,
  term_dealloc,
  0, 
  term_getattr, 0, term_compare, term_repr,
  &term_as_number, &term_as_sequence, 0, /* protocols */
  0, 0,
  term_str, 0, 0, 0,
  Py_TPFLAGS_DEFAULT, /* tp_flags */
  "",                                              /* tp_doc */
  0, 0, 0, 0, 0, 0, 
  term_methods,
  0, 0, 0, 0, 0, 0, offsetof(TPyTerm, attrdict), 0,
  term_alloc, term_new, 0, 0, 0, 0, 0, 0, 0,
};


PyTypeObject *PyTerm_Type = &PyTerm_Type_s;

PyObject *PyTerm_New(SP_term_ref term_ref)
{
  PyObject *self = term_alloc(PyTerm_Type, 0);
  if (self)
    PyTerm_AsTerm(self) = term_ref;
  return self;
}



PyObject *unify(PyObject *, PyObject *args)
{
  SP_term_ref t1, t2;
  if (!PyArg_ParseTuple(args, "O&O&:unify", &cc_Term, &t1, &cc_Term, &t2))
    return NULL;

  return PyInt_FromLong(SP_unify(t1, t2));
}



////////////////////////////////////////////////////////////////////////
//**************    Predicate


void predicate_dealloc(PyObject *self)
{ 
  PyObject_Del(self);
}


PyObject *predicate_repr(PyObject *self)
{
  TPyPredicate *predicate = (TPyPredicate *)(self);

  char *ts = new char[strlen(predicate->name) + 30]; 
  sprintf(ts,"<picstus.Predicate '%s/%i'>", predicate->name, predicate->arity);
  PyObject *res = PyString_FromString(ts);
  delete ts;
  return res;
}


PyObject *predicate_str(PyObject *self)
{
  TPyPredicate *predicate = (TPyPredicate *)(self);

  char *ts = new char[strlen(predicate->name) + 30]; 
  sprintf(ts,"%s/%i", predicate->name, predicate->arity);
  PyObject *res = PyString_FromString(ts);
  delete ts;
  return res;
}


PyMethodDef predicate_methods[] = {
     {NULL, NULL}
};


SP_atom getModule(PyObject *pymodule)
{
  if (!pymodule)
    return SP_atom_from_string("user");
  else if (PyAtom_Check(pymodule))
    return PyAtom_AsAtom(pymodule);
  else if (PyString_Check(pymodule))
    return SP_atom_from_string(PyString_AsString(pymodule));
  else {
    PyErr_Format(PyExc_TypeError, "cannot deduce the module name from an instance of '%s'", pymodule->ob_type->tp_name);
    return 0;
  }
}


PyObject *predicate_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  PyObject *pyname;
  PyObject *pymodule = NULL;
  long arity;
  if (!PyArg_ParseTuple(args, "Oi|O:Predicate", &pyname, &arity, &pymodule))
    return NULL;

  SP_pred_ref pred;
  char *name;

  if (PyString_Check(pyname)) {
    char *module;
    if (!pymodule)
      module = NULL;
    else if (PyString_Check(pymodule))
      module = PyString_AsString(pymodule);
    else if (PyAtom_Check(pymodule))
      module = SP_string_from_atom(PyAtom_AsAtom(pymodule));
    else {
      PyErr_Format(PyExc_TypeError, "cannot deduce the module name from an instance of '%s'", pymodule->ob_type->tp_name);
      return NULL;
    }
    
    name = PyString_AsString(pyname);
    pred = SP_predicate(PyString_AsString(pyname), arity, pymodule ? PyString_AsString(pymodule) : NULL);
  }

  else if (PyAtom_Check(pyname)) {
    SP_atom amodule = getModule(pymodule);
    if (PyErr_Occurred())
      return NULL;
      
    pred = SP_pred(PyAtom_AsAtom(pyname), arity, amodule);
    name = SP_string_from_atom(PyAtom_AsAtom(pyname));
  }
  else
    PYERROR(PyExc_TypeError, "Predicate.__init__ name and module should be strings or atoms", NULL);

  if (!pred) {
    PyErr_Format(PyExc_SystemError, "there's no predicate %s/%i", name, arity);
    return NULL;
  }
    
  PyObject *self = type->tp_alloc(type, 0);
  TPyPredicate *predicate = (TPyPredicate *)(self);
  predicate->predicate = pred;
  predicate->name = name;
  predicate->arity = arity;
 
  predicate->attrdict = NULL;
  assignDictFromDict(self, kwds);

  return self;
}


PyTypeObject PyPredicate_Type_s = {
  PyObject_HEAD_INIT((_typeobject *)&PyType_Type)
  0,
  "pisctus.Predicate",
  sizeof(TPyPredicate), 0,
  predicate_dealloc,
  0, 
  0, 0, 0, predicate_repr,
  0, 0, 0, /* protocols */
  0, 0,
  predicate_str, 0, 0, 0,
  Py_TPFLAGS_DEFAULT, /* tp_flags */
  "",                                              /* tp_doc */
  0, 0, 0, 0, 0, 0, 
  predicate_methods,
  0, 0, 0, 0, 0, 0, offsetof(TPyPredicate, attrdict), 0,
  PyType_GenericAlloc, predicate_new, 0, 0, 0, 0, 0, 0, 0,
};


PyTypeObject *PyPredicate_Type = &PyPredicate_Type_s;

int cc_Predicate(PyObject *obj, void *ptr)
{ if (obj->ob_type != PyPredicate_Type)
    return 0;
  *(SP_pred_ref *)ptr = PyPredicate_AsPredicate(obj);
  return 1;
}



////////////////////////////////////////////////////////////////////////
//**************    Query


TPyQuery *queryStack = NULL;

PyObject *query_alloc(PyTypeObject *type, int size)
{
  PyObject *self = PyType_GenericAlloc(type, size);

  TPyQuery *query = (TPyQuery *)(self);

  query->terms = NULL;
  query->lastTerm = &query->terms;

  query->previousQuery = queryStack;
  queryStack = query;

  query->alive = true;

  return self;
}


void killQuery(TPyQuery *query)
{
  for(TPyTerm *term = query->terms; term; term = term->next)
    term->alive = false;

  query->terms = NULL;
  query->lastTerm = NULL;
  query->alive = false;
}


void killQueriesTo(TPyQuery *query)
{
  for(; queryStack != query; queryStack = queryStack->previousQuery)
    killQuery(queryStack);
  killQuery(query);
  queryStack = query->previousQuery;
}


void query_dealloc(PyObject *self)
{ 
  TPyQuery *query = (TPyQuery *)(self);
  if (query->alive)
    killQueriesTo(query);

  PyObject_Del(self);
}


PyObject *query_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  PyObject *pypred, *pymodule = NULL, *pyterms = NULL;

  if (!PyArg_ParseTuple(args, "O|OO:Query.__init__", &pypred, &pyterms, &pymodule))
    return NULL;
 
  SP_term_ref *termrefs = termArray_from_python(pyterms);
  if (!termrefs)
    return NULL;
  
  // watch the order: termArray_from_python will check whether pyterms is a list!
  int arity = pyterms ? PyList_Size(pyterms) : 0;

  SP_pred_ref predicate;

  if (PyAtom_Check(pypred)) {
    SP_atom amodule = getModule(pymodule);
    if (PyErr_Occurred())
      return 0;
    predicate = SP_pred(PyAtom_AsAtom(pypred), arity, amodule);
    if (!predicate) {
      PyErr_Format(PyExc_SystemError, "there's no predicate %s/%i", SP_string_from_atom(PyAtom_AsAtom(pypred)), arity);
      return NULL;
    }
  }

  else if (PyString_Check(pypred)) {
    if (pymodule && !PyString_Check(pymodule))
      PYERROR(PyExc_TypeError, "if predicate is given as string, so must be the (optional) module", NULL);
    predicate = SP_predicate(PyString_AsString(pypred), arity, pymodule ? PyString_AsString(pymodule) : NULL);
    if (!predicate) {
      PyErr_Format(PyExc_SystemError, "there's no predicate %s/%i", PyString_AsString(pypred), arity);
      return NULL;
    }
  }

  else if (PyPredicate_Check(pypred)) {
    if (((TPyPredicate *)pypred)->arity != arity)
      PYERROR(PyExc_TypeError, "the number of terms mismatches the predicate's arity", NULL);
    if (pymodule)
      PYERROR(PyExc_TypeError, "when predicate is given as predicate, module cannot be specified", NULL);
    predicate = PyPredicate_AsPredicate(pypred);
  }

  else
    PYERROR(PyExc_TypeError, "the first argument to Query.__init__ should be a string, atom or predicate", NULL);


  SP_qid qid = SP_open_query_array(predicate, termrefs);
  if (!qid)
    PYERROR(PyExc_TypeError, "invalid query", NULL);

  PyObject *self = type->tp_alloc(type, 0);
  PyQuery_AsQuery(self) = qid;

  ((TPyQuery *)(self))->attrdict = NULL;
  assignDictFromDict(self, kwds);

  return self;
}


PyObject *query_next_solution(PyObject *self)
{
  CHECK_QUERY_ALIVE(self, NULL);

  if ((TPyQuery *)self != queryStack)
    PYERROR(PyExc_SystemError, "next_solution can only be called for the innermost query", NULL);


  int res = SP_next_solution(PyQuery_AsQuery(self));

  if (res == SP_ERROR) {
    killQueriesTo((TPyQuery *)self);
    setSicstusException();
    return NULL;
  }

  if (res == SP_FAILURE) {
    killQueriesTo((TPyQuery *)self);
    return PyInt_FromLong(0);
  }

  return PyInt_FromLong(1);
}


PyObject *query_cut(PyObject *self)
{
  killQueriesTo((TPyQuery *)self);
  if (SP_cut_query(PyQuery_AsQuery(self)) == SP_ERROR)
    PYERROR(PyExc_SystemError, "error in query", NULL);
  RETURN_NONE;
}


PyObject *query_close(PyObject *self)
{
  killQueriesTo((TPyQuery *)self);
  if (SP_close_query(PyQuery_AsQuery(self)) == SP_ERROR)
    PYERROR(PyExc_SystemError, "error in query", NULL);
  RETURN_NONE;
}




PyMethodDef query_methods[] = {
     {"next_solution", (binaryfunc)query_next_solution, METH_NOARGS, "() -> bool"},
     {"cut", (binaryfunc)query_cut, METH_NOARGS, "() -> None"},
     {"close", (binaryfunc)query_close, METH_NOARGS, "() -> None"},
     {NULL, NULL}
};


PyTypeObject PyQuery_Type_s = {
  PyObject_HEAD_INIT((_typeobject *)&PyType_Type)
  0,
  "pisctus.Query",
  sizeof(TPyQuery), 0,
  query_dealloc,
  0, 
  0, 0, 0, 0,
  0, 0, 0, /* protocols */
  0, 0,
  0, 0, 0, 0,
  Py_TPFLAGS_DEFAULT, /* tp_flags */
  "",                                              /* tp_doc */
  0, 0, 0, 0, 0, 0, 
  query_methods,
  0, 0, 0, 0, 0, 0, offsetof(TPyQuery, attrdict), 0,
  query_alloc, query_new, 0, 0, 0, 0, 0, 0, 0,
};


PyTypeObject *PyQuery_Type = &PyQuery_Type_s;

int cc_Query(PyObject *obj, void *ptr)
{ if (obj->ob_type != PyQuery_Type)
    return 0;
  *(SP_qid *)ptr = PyQuery_AsQuery(obj);
  return 1;
}



PyMethodDef picstusFunctions[]={
     {"unify", (binaryfunc)unify, METH_VARARGS, "(term1, term2) -> int"},
     {NULL, NULL}
};

PyObject *picstusModule;

void readyModule()
{
  picstusModule = Py_InitModule("_picstus", picstusFunctions);

  PyType_Ready(PyPredicate_Type);
  PyType_Ready(PyAtom_Type);
  PyType_Ready(PyQuery_Type);
  PyType_Ready(PyTerm_Type);

  PyModule_AddObject(picstusModule, "Predicate", (PyObject *)PyPredicate_Type);
  PyModule_AddObject(picstusModule, "Atom", (PyObject *)PyAtom_Type);
  PyModule_AddObject(picstusModule, "Query", (PyObject *)PyQuery_Type);
  PyModule_AddObject(picstusModule, "Term", (PyObject *)PyTerm_Type);


  PyModule_AddIntConstant(picstusModule, "VARIABLE", SP_TYPE_VARIABLE);
  PyModule_AddIntConstant(picstusModule, "INTEGER", SP_TYPE_INTEGER);
  PyModule_AddIntConstant(picstusModule, "FLOAT", SP_TYPE_FLOAT);
  PyModule_AddIntConstant(picstusModule, "ATOM", SP_TYPE_ATOM);
  PyModule_AddIntConstant(picstusModule, "COMPOUND", SP_TYPE_COMPOUND);

  emptyListAtom = SP_atom_from_string("[]");
  SP_register_atom(emptyListAtom);

  noneAtom = SP_atom_from_string("none");
  SP_register_atom(noneAtom);


  if (!PyExc_SicstusError) {
    PyExc_SicstusError = PyErr_NewException("_picstus.SicstusError", NULL, NULL);
    PyModule_AddObject(picstusModule, "SicstusError", PyExc_SicstusError);
  }
}


bool isPrologType(PyObject *obj)
{
  return PyTerm_Check(obj) || PyAtom_Check(obj) || PyPredicate_Check(obj) || PyQuery_Check(obj);
}



/* These function can probably be eliminated */

bool term_from_python(PyObject *arg, SP_term_ref &tref)
{
  if (arg->ob_type == PyTerm_Type) {
    SP_put_term(tref, PyTerm_AsTerm(arg));
//    tref = PyTerm_AsTerm(arg);
    return true;
  }
   
  if (PyInt_Check(arg))
    SP_put_integer(tref, PyInt_AsLong(arg));
  else if (PyFloat_Check(arg))
    SP_put_float(tref, PyFloat_AsDouble(arg));
  else if (PyString_Check(arg))
    SP_put_string(tref, PyString_AsString(arg));
  else if (PyList_Check(arg) || PyTuple_Check(arg))
    list_from_python(tref, arg, false);
  else if (arg == Py_None)
    SP_put_atom(tref, noneAtom);
  else {
    PyErr_Format(PyExc_SystemError, "don't know how to convert an object of type '%s' to Prolog", arg->ob_type->tp_name);
    return false;
  }

  return true;
}



PyObject *globals = NULL;
PyObject *builtin = NULL;
PyObject *SyntaxErrorClass = NULL;

PyObject *execfileFunc = NULL;
PyObject *evalFunc = NULL;
PyObject *inputFunc = NULL;

char excbuf[512];

void putErrorString(char *s, ...)
{
  va_list vargs;
  #ifdef HAVE_STDARG_PROTOTYPES
    va_start(vargs, s);
  #else
    va_start(vargs);
  #endif

  vsnprintf(excbuf, 512, s, vargs);
  
  SP_term_ref tref = SP_new_term_ref();
  SP_put_string(tref, excbuf);
  SP_raise_exception(tref);
}

void handleError()
{

    PyObject *ptype, *pvalue, *ptraceback;
    PyErr_Fetch(&ptype, &pvalue, &ptraceback);
    PyErr_Clear();

    PyObject *exc_str = NULL;

    if (ptype == SyntaxErrorClass) {
      PyErr_NormalizeException(&ptype, &pvalue, &ptraceback);
      PyObject *filename = PyObject_GetAttrString(pvalue, "filename");
      if (!filename || (filename == Py_None)) {
        PyObject *offset = PyObject_GetAttrString(pvalue, "offset");
        if (offset) {
          putErrorString("syntax error at or near character #%i", PyInt_AsLong(offset));
          Py_DECREF(offset);
        }
        else
          putErrorString("syntax error");
      }
      else {
        PyObject *lineno = PyObject_GetAttrString(pvalue, "lineno");
        if (lineno) {
          putErrorString("syntax error in file %s, line %i", PyString_AsString(filename), PyInt_AsLong(lineno));
          Py_DECREF(lineno);
        }
        else
          putErrorString("syntax error in file %s", PyString_AsString(filename), PyInt_AsLong(lineno));
      }
      Py_XDECREF(filename);
    }

    else {
      char *err;

      if (PyString_Check(pvalue))
        err = PyString_AsString(pvalue);
      else {
        exc_str = PyObject_Str(pvalue);
        err = PyString_AsString(exc_str);
      }

      if (!err || PyErr_Occurred()) {
        err = "unknown error in Python";
        PyErr_Clear();
      }

      putErrorString(err);
    }

    // shouldn't delete pvalue and exc_str when we still need 'err'
    Py_XDECREF(exc_str);
    Py_XDECREF(ptype);
    Py_XDECREF(pvalue);
    Py_XDECREF(ptraceback);
}


char *stringFromTerm(SP_term_ref term, const char *what)
{
  char *res = stringFromTerm(term);
  if (!res) {
    PyErr_Clear();
    putErrorString("%s should be an atom or a string", what);
  }

  return res;
}



class TTempVar {
public:
  char varname[16];
  SP_term_ref term;

  TTempVar()
  { 
    *varname = 0;
  }


  ~TTempVar()
  {
    if (*varname)
      PyDict_DelItemString(globals, varname);
  }

  void set(const SP_term_ref aterm, PyObject *obj);
};


void TTempVar::set(const SP_term_ref aterm, PyObject *obj)
{
  term = aterm;

  int i = -1;
  do {
    sprintf(varname, "_sp_ps_%i", ++i);
  } while (PyDict_GetItemString(globals, varname));

  PyDict_SetItemString(globals, varname, obj);
  Py_DECREF(obj);
}



class TTempVars {
public:
  TTempVar *vars, *vari, *vare;

  TTempVars();
  ~TTempVars();
  void reserve(const int &size);

  char *assignNewVar(const SP_term_ref &term, PyObject *obj);
  char *assignVar(const SP_term_ref &term);
  char *assignVar(const SP_term_ref &term, PyObject *obj);
};



TTempVars::TTempVars()
: vars(NULL), vari(NULL), vare(NULL)
{}

void TTempVars::reserve(const int &size)
{ 
  if (vars)
    delete vars;

  vari = vars = size ? new TTempVar[size] : NULL;
  vare = vars + size;
}


TTempVars::~TTempVars()
{
  if (vars)
    delete[] vars;
}


char *TTempVars::assignNewVar(const SP_term_ref &term, PyObject *obj)
{
  if (vari == vare) {
    putErrorString("Internal error: TTempVars too small");
    return NULL;
  }

  vari->set(term, obj);
  return (vari++)->varname;
}


char *TTempVars::assignVar(const SP_term_ref &term, PyObject *obj)
{
  for(TTempVar *vi = vars; vi != vari; vi++) {
    if (!SP_compare(vi->term, term))
      return vi->varname;
  }
  
  return assignNewVar(term, obj);
}


char *TTempVars::assignVar(const SP_term_ref &term)
{
  for(TTempVar *vi = vars; vi != vari; vi++) {
    if (!SP_compare(vi->term, term))
      return vi->varname;
  }

  PyObject *obj = term_toPython_low(term);
  if (!obj)
    return NULL;

  char *res = assignNewVar(term, obj);
  if (!res)
    Py_DECREF(obj);
  return res;
}


PyObject *newQuery()
{
  return PyQuery_Type->tp_alloc(PyQuery_Type, 0);
}


void __cdecl python_call(SP_term_ref nameterm, SP_term_ref arglist, SP_term_ref result)
{
  char *funcname = stringFromTerm(nameterm, "function name");
  if (!funcname)
    return;

  PyObject *pyfunc = PyDict_GetItemString(globals, funcname);
  if (!pyfunc) 
    pyfunc = PyDict_GetItemString(builtin, funcname);
  if (!pyfunc)
    pyfunc = PyRun_String(funcname, Py_eval_input, globals, globals);
  if (!pyfunc) {
    PyErr_Clear();
    putErrorString("python function '%s' not found", funcname);
    return;
  }

  if (!PyCallable_Check(pyfunc)) {
    putErrorString("python object '%s' is not callable", funcname);
    return;
  }
        
  PyObject *thisQuery = newQuery();

  PyObject *args = list_to_python(arglist, true);
  if (!args) {
    // watch the order: handleError before DECREF(thisQuery)
    handleError();
    Py_DECREF(thisQuery);
    return;
  }

  
  if ((PyTuple_Size(args)==1) && ((pyfunc == execfileFunc) || (pyfunc == evalFunc))) {
    PyObject *oldargs = args;
    args = Py_BuildValue("OOO", PyTuple_GET_ITEM(args, 0), globals, globals);
    Py_DECREF(oldargs);
  }

  PyObject *res = PyObject_CallObject(pyfunc, args);
  Py_DECREF(args);

  if (!res || PyErr_Occurred() || !term_from_python(res, result)) {
    handleError();
    Py_DECREF(thisQuery);
    return;
  }

  Py_DECREF(res);
  Py_DECREF(thisQuery);
}



void __cdecl python_set(SP_term_ref nameterm, SP_term_ref term) 
{
  char *name = stringFromTerm(nameterm, "variable to set");
  if (!name)
    return;

  /* allow passing unconverted terms -- if the object's __setattr__ is suitably
     defined, it may still do something with it */
  PyObject *thisQuery = newQuery();

  PyObject *obj = term_toPython_low(term);
  if (!obj) {
    Py_DECREF(thisQuery);
    return;
  }

  TTempVar tempvar;
  tempvar.set(term, obj);

  char *wholes = new char[strlen(name)+15];
  sprintf(wholes, "%s=%s", name, tempvar.varname);

  PyObject *res = PyRun_String(wholes, Py_file_input, globals, globals);

  delete wholes;

  Py_DECREF(thisQuery);

  if (!res || PyErr_Occurred())
    handleError();

  Py_XDECREF(res);
}






char *reparseExpression(SP_term_ref args, TTempVars &tempVars, const char *errObject)
{
  if (SP_is_compound(args)) {
    SP_atom functor;
    int arity;
    SP_get_functor(args, &functor, &arity);
    char *sfunctor = SP_string_from_atom(functor);

    if (!strcmp(sfunctor, "<") && (arity == 2)) {
      SP_term_ref formatStr = SP_new_term_ref(), formatData = SP_new_term_ref();
      SP_get_arg(1, args, formatStr);
      SP_get_arg(2, args, formatData);

      char *expr = stringFromTerm(formatStr, errObject);
      if (!expr)
        return NULL;

      char *ei;
      int nargs = 0;
      for(ei = expr; *ei; ei++)
        if (*ei == '%') {
          ei++;
          if ((*ei == 'v') || (*ei == 't'))
            nargs++;
          if (!*ei)
            break;
        }

      int size;
      SP_term_ref *termArray;
      if (termIsList(formatData))
        termArray = list_to_array(formatData, size);
      else {
        termArray = new SP_term_ref[1];
        *termArray = SP_new_term_ref();
        SP_put_term(*termArray, formatData);
        size = 1;
      }

      SP_term_ref *termi = termArray, *terme = termArray + size;

      tempVars.reserve(nargs);
      TString reparsedS = TString();
  
      for(ei = expr; *ei; *ei++) {
        if (*ei != '%') {
          reparsedS.putc(*ei);
          continue;
        }

        ei++;

        if (*ei == '%') {
          reparsedS.putc('%');
          continue;
        }

        if (termi == terme) {
          putErrorString("not enough arguments for the format string");
          delete termArray;
          return NULL;
        }

        char *str;

        if ((*ei == 's') && SP_is_atom(*termi) && SP_get_string(*termi, &str) && !strcmp(str, "[]")) {
          termi++;
          continue; // we insert an empty string by not inserting it
        }
 
        if (*ei == 'v') {
          str = tempVars.assignVar(*(termi++));
          if (!str) {
            delete termArray;
            return NULL;
          }

          reparsedS.puts(str);
          continue;
        }


        if (*ei == 't') {
          PyObject *obj = PyTerm_New(*termi);
          if (!obj) {
            handleError();
            return NULL;
          }

          str = tempVars.assignVar(*termi++, obj);

          reparsedS.puts(str);
          continue;
        }
          

        if (*ei == 's') {
          if (SP_is_integer(*termi))
            *ei = 'i';

          else if ((*ei == 's') && SP_is_float(*termi))
            *ei = 'f';

          else {
            if (SP_is_list(*termi)) {
              str = list_to_chars(*termi++);
              reparsedS.puts(str);
              delete str;
              continue;
            }

            else if (SP_is_atom(*termi)) {
              SP_get_string(*termi++, &str);
              reparsedS.puts(str);
              continue;
            }

            putErrorString("invalid argument given for a string");
            delete termArray;
            return NULL;
          }
        }

        if ((*ei == 'i') || (*ei == 'f')) {
          char istr[128];
          if (SP_is_float(*termi)) {
            double i;
            SP_get_float(*termi++, &i);
            if (*ei == 'i')
              sprintf(istr, "%i", int(i));
            else
              sprintf(istr, "%f", i);
          }

          else if (SP_is_integer(*termi)) {
            long i;
            SP_get_integer(*termi++, &i);
            sprintf(istr, "%i", i);
          }

          else {
            putErrorString("invalid argument for %%i");
            delete termArray;
            return NULL;
          }

          reparsedS.puts(istr);
          continue;
        }


        putErrorString("invalid format character (%%%c)", *ei);
        delete termArray;
        return NULL;
      }

      if (termi != terme) {
        putErrorString("too many arguments for the format string");
        delete termArray;
        return NULL;
      }

      delete termArray;
      return reparsedS.releaseString();
    }
  }

  return stringFromTerm(args, "expression");
}


void __cdecl python_eval(SP_term_ref args, SP_term_ref result)
{
  PyObject *thisQuery = newQuery();
  TTempVars tempVars;
  char *s = reparseExpression(args, tempVars, "expression");
  if (!s) {
    Py_DECREF(thisQuery);
    return;
  }

  PyObject *res = PyRun_String(s, Py_eval_input, globals, globals);

  if (!res || PyErr_Occurred() || !term_from_python(res, result))
    handleError(); 

  Py_DECREF(thisQuery);
  Py_XDECREF(res);
}



void __cdecl python_exec(SP_term_ref args)
{
  PyObject *thisQuery = newQuery();
  TTempVars tempVars;
  char *s = reparseExpression(args, tempVars, "statement");
  if (!s) {
    Py_DECREF(thisQuery);
    return;
  }

  PyObject *res = PyRun_String(s, Py_file_input, globals, globals);

  if (!res || PyErr_Occurred())
    handleError();

  Py_XDECREF(res);
  Py_DECREF(thisQuery);
}



/* This function is called in both cases, if the initial application is Python or Sicstus.
   It must not initialize Python if Python is the outer application. */

// executable "c:\Program Files\SICStus Prolog 3.12.0\bin\sicstus.exe"

void init(int when) {
  // I don't know why I have to do this, but it seems I have to...
  sp_GlobalSICStus_picstus = SP_get_dispatch(NULL);

  // we're not here for the first time, everything is already done
  if (globals)
    return;

  // python may be already initialized if Python is actually the initial session
  if (!pythonInitialized) {
    Py_Initialize();
    // we'll set the flag later
  }


	PyObject *m;
  
  m = PyImport_AddModule("__main__");
	globals = PyModule_GetDict(m);

  m = PyImport_AddModule("__builtin__");
  builtin = PyModule_GetDict(m);

  m = PyImport_AddModule("exceptions");
  SyntaxErrorClass = PyDict_GetItemString(PyModule_GetDict(m), "SyntaxError");

  execfileFunc = PyDict_GetItemString(builtin, "execfile");
  evalFunc = PyDict_GetItemString(builtin, "eval");
  inputFunc = PyDict_GetItemString(builtin, "input");;

  if (!pythonInitialized) {
    readyModule();
    PyDict_SetItemString(globals, "_picstus", picstusModule);

    pythonInitialized = true;
  }
}

void deinit(int when) {
  // placeholder for anything we'd need in the future
}




SP_MainFun *sp_pre_linkage[] = {0};
char *sp_pre_map[] = {0};    


/* This function is called only if Python is the parent application.
   If it's Sicstus, the module is prepared and 'manually' added to the
   dictionary (is this hack still needed?!). */
extern "C" __declspec(dllexport) void init_picstus()
{
   sp_GlobalSICStus_picstus = SP_get_dispatch(NULL);
   if (SP_initialize(0, NULL, NULL) == SP_ERROR) {
    PyErr_Format(PyExc_SystemError, "cannot initialize Sicstus Prolog engine");
    return;
   };
   
   readyModule();

  pythonInitialized = true;
}

extern "C" __declspec(dllexport) void initpython()
{ init_picstus(); }
