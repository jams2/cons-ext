/****
 * Consider these all "maybes" for now:
 *
 * TODO: provide map and reduce (foldl,foldr) functions
 * TODO: add to_str, to_tuple, to_bytes (?)
 * TODO: make cons iterable, so it can be unpacked into a 2-tuple
 * TODO: provide a to_list_iter method, which iterates over each member of a proper cons list
 *       - see tupleobject.c, PyTupleIter_Type
 *
 ****/

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <structmember.h>
#include <stdbool.h>

#define IS_LIST(ptr) (((ConsObject *)ptr)->is_list)
#define CAR(ptr) (((ConsObject *)ptr)->head)
#define CDR(ptr) (((ConsObject *)ptr)->tail)
#define SET_CAR(op, value) ((ConsObject *)op)->head = value
#define SET_CDR(op, value) ((ConsObject *)op)->tail = value
#define SET_IS_LIST(op, b) ((ConsObject *)op)->is_list = b
#define Cons_NEW(cons_type) PyObject_GC_New(ConsObject, (PyTypeObject *)cons_type)
#define Cons_NEW_PY(cons_type) \
    (PyObject *)PyObject_GC_New(ConsObject, (PyTypeObject *)cons_type)

// ((PyObject *)x, (PyObject *)ConsType, (PyObject *)nil) -> (PyObject *)y
typedef PyObject *(*cmapfn_t)(PyObject *, PyObject *, PyObject *);

static struct PyModuleDef consmodule;

typedef struct {
    PyObject *NilType;
    PyObject *nil;
    PyObject *ConsType;
} consmodule_state;

/* The Nil type */
typedef struct {
    PyObject_HEAD
} NilObject;

static int
Nil_traverse(NilObject *self, visitproc visit, void *arg)
{
    Py_VISIT(Py_TYPE(self));
    return 0;
}

PyObject *
Nil_repr(PyObject *self)
{
    return PyUnicode_FromString("nil()");
}

static PyObject *
Nil_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    if (PyTuple_Size(args) || (kwds && PyDict_Size(kwds))) {
        PyErr_SetString(PyExc_TypeError, "nil() takes no arguments");
        return NULL;
    }

    consmodule_state *state = PyType_GetModuleState(type);
    if (state == NULL)
        return NULL;
    PyObject *self = state->nil;
    Py_INCREF(self);
    return self;
}

static int
Nil_bool(PyObject *self)
{
    return 0;
}

static PyObject *
Nil_to_list(PyObject *self, PyTypeObject *defining_class, PyObject *const *args,
            Py_ssize_t nargs, PyObject *kwnames)
{
    if (nargs != 0) {
        PyErr_SetString(PyExc_TypeError, "expected zero arguments");
        return NULL;
    }
    return PyList_New(0);
}

PyDoc_STRVAR(Nil_doc, "Get the singleton nil object");
PyDoc_STRVAR(Nil_to_list_doc, "Convert nil to an empty Python list");

static PyMethodDef Nil_methods[] = {
    {"to_list", (PyCFunction)Nil_to_list, METH_METHOD | METH_FASTCALL | METH_KEYWORDS,
     Nil_to_list_doc},
    {NULL, NULL}};

static void
Nil_dealloc(NilObject *self)
{
    if (self != NULL) {
        PyObject_ClearWeakRefs((PyObject *)self);
        Py_TYPE(self)->tp_free(self);
    }
}

static PyType_Slot Nil_Type_Slots[] = {
    {Py_tp_doc, (void *)Nil_doc}, {Py_tp_new, Nil_new},
    {Py_tp_repr, Nil_repr},       {Py_tp_traverse, Nil_traverse},
    {Py_nb_bool, Nil_bool},       {Py_tp_methods, Nil_methods},
    {Py_tp_dealloc, Nil_dealloc}, {0, NULL},
};

static PyType_Spec Nil_Type_Spec = {
    .name = "fastcons.nil",
    .basicsize = sizeof(NilObject),
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_IMMUTABLETYPE | Py_TPFLAGS_MANAGED_WEAKREF,
    .slots = Nil_Type_Slots,
};

/* The Cons type */
typedef struct {
    PyObject_HEAD PyObject *head;
    PyObject *tail;
    bool is_list;
} ConsObject;

PyObject *
Cons_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    consmodule_state *state = PyType_GetModuleState(type);
    if (state == NULL)
        return NULL;
    PyTypeObject *cons_type = (PyTypeObject *)state->ConsType;

    PyObject *head = NULL, *tail = NULL;
    static char *kwlist[] = {"head", "tail", NULL};

    // Parse arguments BEFORE allocating the object, so we don't need
    // to decref etc. if this fails.
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "OO", kwlist, &head, &tail))
        return NULL;

    // Now allocate the object
    ConsObject *self = Cons_NEW(cons_type);
    if (self == NULL)
        return NULL;

    // Initialize fields
    self->head = NULL;
    self->tail = NULL;
    self->is_list = false;

    PyObject_GC_Track(self);

    if (Py_Is(tail, state->nil))
        self->is_list = true;
    else if (Py_IS_TYPE(tail, cons_type))
        self->is_list = IS_LIST(tail);

    Py_INCREF(head);
    self->head = head;
    Py_INCREF(tail);
    self->tail = tail;

    return (PyObject *)self;
};

static int
Cons_traverse(ConsObject *self, visitproc visit, void *arg)
{
    Py_VISIT(Py_TYPE(self));
    Py_VISIT(self->head);
    Py_VISIT(self->tail);
    return 0;
}

int
Cons_clear(PyObject *self)
{
    Py_CLEAR(CAR(self));
    Py_CLEAR(CDR(self));
    return 0;
}

void
Cons_dealloc(ConsObject *self)
{
    PyObject_ClearWeakRefs((PyObject *)self);
    PyObject_GC_UnTrack(self);
    Py_TRASHCAN_BEGIN(self, Cons_dealloc);
    Cons_clear((PyObject *)self);
    Py_TYPE(self)->tp_free(self);
    Py_TRASHCAN_END;
}

static inline PyObject *
identity(PyObject *op, PyObject *cons_type, PyObject *nil)
{
    return op;
}

PyObject *
Cons_from_fast_with(PyObject *xs, PyObject *cons_type, PyObject *nil, cmapfn_t f)
{
    Py_ssize_t len = PySequence_Fast_GET_SIZE(xs);

    PyObject *result = nil;
    Py_INCREF(nil);
    PyObject *item = NULL, *current = NULL, *tmp = NULL;
    for (Py_ssize_t i = len - 1; i >= 0; i--) {
        item = PySequence_Fast_GET_ITEM(xs, i);
        Py_INCREF(item);
        current = Cons_NEW_PY(cons_type);
        if (current == NULL) {
            Py_DECREF(item);
            Py_DECREF(result);
            return NULL;
        }

        tmp = f(item, cons_type, nil);
        if (tmp == NULL) {
            Py_DECREF(item);
            Py_DECREF(result);
            return NULL;
        }
        SET_CAR(current, tmp);
        SET_CDR(current, result);
        PyObject_GC_Track(current);
        SET_IS_LIST(current, true);
        result = current;
    }

    return result;
}

PyObject *
Cons_from_gen_with(PyObject *xs, PyObject *cons_type, PyObject *nil, cmapfn_t f)
{
    PyObject *head = NULL, *current = NULL, *item = NULL, *tmp = NULL;
    while ((item = PyIter_Next(xs)) != NULL) {
        tmp = Cons_NEW_PY(cons_type);
        if (tmp == NULL) {
            Py_DECREF(item);
            return NULL;
        }

        PyObject *_item = f(item, cons_type, nil);
        if (_item == NULL) {
            Py_DECREF(item);
            return NULL;
        }
        SET_CAR(tmp, _item);
        SET_IS_LIST(tmp, true);

        if (head == NULL)
            head = current = tmp;
        else {
            SET_CDR(current, tmp);
            PyObject_GC_Track(current);
            current = tmp;
        }
    }

    if (current == NULL) {
        if (PyErr_Occurred())
            return NULL;
        else {
            /* Empty generator */
            Py_INCREF(nil);
            return nil;
        }
    }

    Py_IncRef(nil);
    SET_CDR(current, nil);
    PyObject_GC_Track(current);
    return head;
}

PyObject *
Cons_from_xs(PyObject *self, PyTypeObject *defining_class, PyObject *const *args,
             Py_ssize_t nargs, PyObject *kwnames)
{
    if (nargs != 1) {
        PyErr_SetString(PyExc_TypeError, "cons.from_xs takes exactly one argument");
        return NULL;
    }

    consmodule_state *state = PyType_GetModuleState(defining_class);
    if (state == NULL)
        return NULL;

    PyObject *xs = args[0], *result = NULL;
    if (PyGen_Check(xs))
        result = Cons_from_gen_with(xs, state->ConsType, state->nil, &identity);
    else if ((xs = PySequence_Fast(xs, "Expected a sequence or iterable")) != NULL)
        result = Cons_from_fast_with(xs, state->ConsType, state->nil, &identity);
    else
        return NULL;

    Py_DECREF(xs);
    return result;
}

static PyObject *
lift(PyObject *, PyObject *, PyObject *);

static PyObject *
lift_dict(PyObject *op, PyObject *cons_type, PyObject *nil)
{
    Py_ssize_t nitems = PyObject_Size(op);
    if (nitems < 0)
        return NULL;
    else if (nitems == 0) {
        Py_INCREF(nil);
        return nil;
    }

    PyObject *key, *value;
    Py_ssize_t pos = 0;
    PyObject *head = NULL, *current = NULL;
    while (PyDict_Next(op, &pos, &key, &value)) {
        PyObject *car = NULL, *cdr = NULL, *tmp = NULL;
        if ((car = lift(key, cons_type, nil)) == NULL)
            return NULL;
        if ((cdr = lift(value, cons_type, nil)) == NULL) {
            Py_DECREF(car);
            return NULL;
        }

        PyObject *pair = Cons_NEW_PY(cons_type);
        if (pair == NULL) {
            Py_DECREF(car);
            Py_DECREF(cdr);
            return NULL;
        }

        /* car and cdr returned from recursive lift calls, so refcount already set */
        SET_CAR(pair, car);
        SET_CDR(pair, cdr);
        SET_IS_LIST(pair, false);
        PyObject_GC_Track(pair);

        tmp = Cons_NEW_PY(cons_type);
        if (tmp == NULL) {
            Py_DECREF(pair);
            Py_XDECREF(head);
            return NULL;
        }
        SET_CAR(tmp, pair);
        SET_IS_LIST(tmp, true);

        if (head == NULL)
            head = current = tmp;
        else {
            SET_CDR(current, tmp);
            PyObject_GC_Track(current);
            current = tmp;
        }
    }

    /* If PyDict_Next failed on the first iteration, current will be NULL */
    if (current == NULL)
        return NULL;

    Py_IncRef(nil);
    SET_CDR(current, nil);
    PyObject_GC_Track(current);
    return head;
}

static PyObject *
lift(PyObject *op, PyObject *cons_type, PyObject *nil)
{
    if (PyDict_Check(op))
        return lift_dict(op, cons_type, nil);
    else if (PyGen_Check(op))
        return Cons_from_gen_with(op, cons_type, nil, &lift);
    else if (PyList_Check(op) || PyTuple_Check(op))
        return Cons_from_fast_with(op, cons_type, nil, &lift);
    else {
        Py_INCREF(op);
        return op;
    }
}

PyObject *
Cons_lift(PyObject *self, PyTypeObject *defining_class, PyObject *const *args,
          Py_ssize_t nargs, PyObject *kwnames)
{
    if (nargs != 1) {
        PyErr_SetString(PyExc_TypeError, "cons.lift takes exactly one argument");
        return NULL;
    }

    consmodule_state *state = PyType_GetModuleState(defining_class);
    if (state == NULL)
        return NULL;

    PyObject *op = args[0];
    return lift(op, state->ConsType, state->nil);
}

Py_ssize_t
cons_len(PyObject *op, PyObject *nil)
{
    Py_ssize_t len = 0;
    for (PyObject *pair = op; !Py_Is(pair, nil); len++, pair = CDR(pair))
        ;
    return len;
}

PyObject *
Cons_to_list(PyObject *self, PyTypeObject *defining_class, PyObject *const *args,
             Py_ssize_t nargs, PyObject *kwnames)
{
    if (nargs != 0) {
        PyErr_SetString(PyExc_TypeError, "expected zero arguments");
        return NULL;
    }
    else if (!IS_LIST(self)) {
        PyErr_SetString(PyExc_ValueError, "expected proper cons list");
        return NULL;
    }
    consmodule_state *state = PyType_GetModuleState(defining_class);
    if (state == NULL)
        return NULL;

    Py_ssize_t len = cons_len(self, state->nil);
    PyObject *list = PyList_New(len);
    PyObject *next = self, *head = NULL;
    for (Py_ssize_t i = 0; i < len; i++, next = CDR(next)) {
        head = CAR(next);
        Py_INCREF(head);  // PyList_SET_ITEM steals a reference
        PyList_SET_ITEM(list, i, head);
    }
    return list;
}

PyObject *
Cons_repr(PyObject *self)
{
    _PyUnicodeWriter writer;
    Py_ssize_t i;
    PyObject *next = self;
    PyObject *m = PyType_GetModuleByDef(Py_TYPE(self), &consmodule);
    if (m == NULL) {
        return NULL;
    }
    consmodule_state *state = PyModule_GetState(m);
    if (state == NULL)
        return NULL;

    PyTypeObject *cons = (PyTypeObject *)state->ConsType;

    i = Py_ReprEnter(self);
    if (i != 0)
        return i > 0 ? PyUnicode_FromFormat("...") : NULL;

    _PyUnicodeWriter_Init(&writer);
    writer.overallocate = 1;
    writer.min_length = 3;  // "(_)"
    if (_PyUnicodeWriter_WriteChar(&writer, '(') < 0)
        goto error;

    PyObject *head = NULL, *repr = NULL, *tail = NULL;
    while (Py_IS_TYPE(next, cons)) {
        head = CAR(next);
        repr = PyObject_Repr(head);
        if (repr == NULL)
            goto error;
        if (_PyUnicodeWriter_WriteStr(&writer, repr) < 0) {
            Py_DECREF(repr);
            goto error;
        }
        Py_DECREF(repr);

        tail = CDR(next);
        if (Py_Is(tail, state->nil))
            break;
        else if (!Py_IS_TYPE(tail, cons)) {
            if (_PyUnicodeWriter_WriteASCIIString(&writer, " . ", 3) < 0)
                goto error;
            repr = PyObject_Repr(tail);
            if (repr == NULL)
                goto error;
            if (_PyUnicodeWriter_WriteStr(&writer, repr) < 0) {
                Py_DECREF(repr);
                goto error;
            }
            Py_DECREF(repr);
            break;
        }
        if (_PyUnicodeWriter_WriteChar(&writer, ' ') < 0)
            goto error;
        next = tail;
    }

    writer.overallocate = 0;
    if (_PyUnicodeWriter_WriteChar(&writer, ')') < 0)
        goto error;
    Py_ReprLeave(self);
    return _PyUnicodeWriter_Finish(&writer);

error:
    _PyUnicodeWriter_Dealloc(&writer);
    Py_ReprLeave(self);
    return NULL;
}

PyObject *
Cons_richcompare(PyObject *self, PyObject *other, int op)
{
    consmodule_state *state = PyType_GetModuleState(Py_TYPE(self));
    if (state == NULL)
        return NULL;

    PyObject *nil = state->nil;
    PyTypeObject *cons = (PyTypeObject *)state->ConsType;
    if (!Py_IS_TYPE(other, cons))
        Py_RETURN_NOTIMPLEMENTED;

    PyObject *this = self, *that = other;
    /* cdr down the list until comparison fails or either object is not a cons */
    while (Py_IS_TYPE(this, cons) && Py_IS_TYPE(that, cons)) {
        int cmp = PyObject_RichCompareBool(CAR(this), CAR(that), op);
        if (cmp < 0)
            return NULL;
        else if (cmp && op == Py_NE)
            Py_RETURN_TRUE;
        else if (!cmp && op != Py_NE)
            Py_RETURN_FALSE;
        else {
            this = CDR(this);
            that = CDR(that);
        }
    }

    if (Py_Is(this, nil) && Py_Is(that, nil)) {
        // In the case of a proper list, disregard comparison of the terminating element
        if (op == Py_NE)
            Py_RETURN_FALSE;
        else
            Py_RETURN_TRUE;
    }
    return PyObject_RichCompare(this, that, op);
}

/* Simplified xxHash - see https://github.com/Cyan4973/xxHash/blob/master/doc/xxhash_spec.md
   and tupleobject.c
*/
#if SIZEOF_PY_UHASH_T > 4
#define _PyHASH_XXPRIME_1 ((Py_uhash_t)11400714785074694791ULL)
#define _PyHASH_XXPRIME_2 ((Py_uhash_t)14029467366897019727ULL)
#define _PyHASH_XXPRIME_5 ((Py_uhash_t)2870177450012600261ULL)
#define _PyHASH_XXROTATE(x) ((x << 31) | (x >> 33)) /* Rotate left 31 bits */
#else
#define _PyHASH_XXPRIME_1 ((Py_uhash_t)2654435761UL)
#define _PyHASH_XXPRIME_2 ((Py_uhash_t)2246822519UL)
#define _PyHASH_XXPRIME_5 ((Py_uhash_t)374761393UL)
#define _PyHASH_XXROTATE(x) ((x << 13) | (x >> 19)) /* Rotate left 13 bits */
#endif

static Py_hash_t
Cons_hash(ConsObject *cons)
{
    PyObject *objs[2] = {cons->head, cons->tail};
    Py_uhash_t acc = _PyHASH_XXPRIME_5;

    for (size_t i = 0; i < 2; i++) {
        Py_uhash_t lane = PyObject_Hash(objs[i]);
        if (lane == (Py_uhash_t)-1)
            return -1;
        acc += lane * _PyHASH_XXPRIME_2;
        acc = _PyHASH_XXROTATE(acc);
        acc *= _PyHASH_XXPRIME_1;
    }
    /* Adding the length complicates matters (do cons(1, 2) and cons(1, nil()) have the same
       length wrt hashing?) so leave it - the xxHash spec allows length to be zero.
    */
    return acc;
}

static PyMemberDef Cons_members[] = {
    {"head", T_OBJECT_EX, offsetof(ConsObject, head), READONLY, "cons head"},
    {"tail", T_OBJECT_EX, offsetof(ConsObject, tail), READONLY, "cons tail"},
    {NULL},
};

PyDoc_STRVAR(from_xs_doc, "Create a cons list from a sequence or iterable");
PyDoc_STRVAR(to_list_doc, "Convert a proper const list to a Python list");
PyDoc_STRVAR(lift_doc,
             "Recursively convert a Python sequence and its sub-sequences to a conses");

static PyMethodDef Cons_methods[] = {
    {"from_xs", (PyCFunction)Cons_from_xs,
     METH_METHOD | METH_FASTCALL | METH_KEYWORDS | METH_CLASS, from_xs_doc},
    {"to_list", (PyCFunction)Cons_to_list, METH_METHOD | METH_FASTCALL | METH_KEYWORDS,
     to_list_doc},
    {"lift", (PyCFunction)Cons_lift, METH_METHOD | METH_FASTCALL | METH_KEYWORDS | METH_CLASS,
     lift_doc},
    {NULL},
};

PyDoc_STRVAR(cons_doc, "Construct a new immutable pair");

static PyType_Slot Cons_Type_Slots[] = {
    {Py_tp_doc, (void *)cons_doc},
    {Py_tp_dealloc, Cons_dealloc},
    {Py_tp_new, Cons_new},
    {Py_tp_members, Cons_members},
    {Py_tp_traverse, Cons_traverse},
    {Py_tp_clear, Cons_clear},
    {Py_tp_repr, Cons_repr},
    {Py_tp_methods, Cons_methods},
    {Py_tp_richcompare, Cons_richcompare},
    {Py_tp_hash, Cons_hash},
    {0, NULL},
};

static PyType_Spec Cons_Type_Spec = {
    .name = "fastcons.cons",
    .basicsize = sizeof(ConsObject),
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_IMMUTABLETYPE | Py_TPFLAGS_HAVE_GC |
             Py_TPFLAGS_MANAGED_WEAKREF,
    .slots = Cons_Type_Slots,
};

/* module level functions */
PyDoc_STRVAR(consmodule_assoc_doc,
             "assoc(object, alist)\n\
\n\
Return the first pair in alist whose car is equal to object. Return\n\
nil() if object is not found.");

PyObject *
consmodule_assoc(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    if (nargs != 2) {
        PyErr_SetString(PyExc_TypeError, "assoc requires exactly two positional arguments");
        return NULL;
    }
    PyObject *object = args[0];
    PyObject *alist = args[1];

    consmodule_state *state = PyModule_GetState(module);
    if (state == NULL)
        return NULL;

    if (Py_Is(alist, state->nil)) {
        Py_INCREF(state->nil);
        return state->nil;
    }
    else if (!Py_IS_TYPE(alist, (PyTypeObject *)state->ConsType) || !IS_LIST(alist)) {
        PyErr_SetString(
            PyExc_ValueError,
            "argument 'alist' to assoc must be a cons list of cons pairs, or nil()");
        return NULL;
    }

    for (PyObject *pair = CAR(alist); !Py_Is(alist, state->nil);
         alist = CDR(alist), pair = CAR(alist)) {
        if (!Py_IS_TYPE(pair, (PyTypeObject *)state->ConsType)) {
            PyErr_SetString(PyExc_ValueError,
                            "'alist' is not a properly formed association list");
            return NULL;
        }
        else if (PyObject_RichCompareBool(object, CAR(pair), Py_EQ)) {
            Py_INCREF(pair);
            return pair;
        }
    }

    Py_INCREF(state->nil);
    return state->nil;
}

PyDoc_STRVAR(consmodule_assp_doc,
             "assp(predicate, alist)\n\
\n\
Return the first pair in alist for which the result of calling 'predicate'\n\
on its car is truthy.\n\
\n\
'predicate' must be a function that takes a single argument.");

PyObject *
consmodule_assp(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    if (nargs != 2) {
        PyErr_SetString(PyExc_TypeError, "assp requires exactly two positional arguments");
        return NULL;
    }
    PyObject *predicate = args[0];
    PyObject *alist = args[1];

    consmodule_state *state = PyModule_GetState(module);
    if (state == NULL)
        return NULL;

    if (Py_Is(alist, state->nil)) {
        Py_INCREF(state->nil);
        return state->nil;
    }
    else if (!Py_IS_TYPE(alist, (PyTypeObject *)state->ConsType) || !IS_LIST(alist)) {
        PyErr_SetString(
            PyExc_ValueError,
            "argument 'alist' to assp must be a cons list of cons pairs, or nil()");
        return NULL;
    }
    else if (!PyFunction_Check(predicate)) {
        PyErr_SetString(PyExc_ValueError, "argument 'predicate' to assp must be a function");
        return NULL;
    }

    for (PyObject *pair = CAR(alist); !Py_Is(alist, state->nil);
         alist = CDR(alist), pair = CAR(alist)) {
        if (!Py_IS_TYPE(pair, (PyTypeObject *)state->ConsType)) {
            PyErr_SetString(PyExc_ValueError,
                            "'alist' is not a properly formed association list");
            return NULL;
        }
        PyObject *result = PyObject_CallOneArg(predicate, CAR(pair));
        if (result == NULL)
            return NULL;
        else if (PyObject_IsTrue(result)) {
            Py_INCREF(pair);
            return pair;
        }
    }

    Py_INCREF(state->nil);
    return state->nil;
}

/* module initialisation */
static int
consmodule_exec(PyObject *m)
{
    consmodule_state *state = PyModule_GetState(m);
    if (state == NULL)
        return -1;

    state->ConsType = PyType_FromModuleAndSpec(m, &Cons_Type_Spec, NULL);
    if (state->ConsType == NULL)
        return -1;
    if (PyModule_AddType(m, (PyTypeObject *)state->ConsType) < 0)
        return -1;
    PyObject *match_args = PyTuple_New(2);
    if (match_args == NULL)
        return -1;
    PyTuple_SET_ITEM(match_args, 0, PyUnicode_FromString("head"));
    PyTuple_SET_ITEM(match_args, 1, PyUnicode_FromString("tail"));
    if (PyDict_SetItemString(((PyTypeObject *)state->ConsType)->tp_dict, "__match_args__",
                             match_args) < 0) {
        Py_DECREF(match_args);
        return -1;
    }
    Py_DECREF(match_args);

    state->NilType = PyType_FromModuleAndSpec(m, &Nil_Type_Spec, NULL);
    if (state->NilType == NULL)
        return -1;
    if (PyModule_AddType(m, (PyTypeObject *)state->NilType) < 0)
        return -1;

    PyObject *nil =
        ((PyTypeObject *)state->NilType)->tp_alloc((PyTypeObject *)state->NilType, 0);
    if (nil != NULL) {
        PyObject_GC_Track(nil);
    }
    state->nil = nil;
    return 0;
}

static PyModuleDef_Slot consmodule_slots[] = {
    {Py_mod_exec, consmodule_exec},
    {0, NULL},
};

static int
consmodule_traverse(PyObject *m, visitproc visit, void *arg)
{
    consmodule_state *state = PyModule_GetState(m);
    Py_VISIT(state->ConsType);
    Py_VISIT(state->NilType);
    Py_VISIT(state->nil);
    return 0;
}

static int
consmodule_clear(PyObject *m)
{
    consmodule_state *state = PyModule_GetState(m);
    Py_CLEAR(state->ConsType);
    Py_CLEAR(state->NilType);
    Py_CLEAR(state->nil);
    return 0;
}

static PyMethodDef consmodule_methods[] = {
    {"assoc", (PyCFunction)consmodule_assoc, METH_FASTCALL, consmodule_assoc_doc},
    {"assp", (PyCFunction)consmodule_assp, METH_FASTCALL, consmodule_assp_doc},
    {NULL, NULL},
};

static struct PyModuleDef consmodule = {
    PyModuleDef_HEAD_INIT,
    .m_name = "fastcons",
    .m_doc = "Module exporting cons type",
    .m_size = sizeof(consmodule_state),
    .m_methods = consmodule_methods,
    .m_slots = consmodule_slots,
    .m_traverse = consmodule_traverse,
    .m_clear = consmodule_clear,
};

PyMODINIT_FUNC
PyInit_fastcons(void)
{
    return PyModuleDef_Init(&consmodule);
}
