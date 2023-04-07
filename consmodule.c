#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <structmember.h>

#define DECREF_AND_NULLIFY(ptr) \
    do {                        \
        Py_DECREF(ptr);         \
        ptr = NULL;             \
    } while (0)

#define XDECREF_AND_NULLIFY(ptr) \
    do {                         \
        Py_XDECREF(ptr);         \
        ptr = NULL;              \
    } while (0)

#define IS_LIST(ptr) (((ConsObject *)ptr)->list_len > 0)
#define LIST_LEN(ptr) (((ConsObject *)ptr)->list_len)
#define CAR(ptr) (((ConsObject *)ptr)->head)
#define CDR(ptr) (((ConsObject *)ptr)->tail)

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

PyDoc_STRVAR(Nil_doc, "Get the singleton nil object");

static PyType_Slot Nil_Type_Slots[] = {
    {Py_tp_doc, (void *)Nil_doc},   {Py_tp_new, Nil_new},   {Py_tp_repr, Nil_repr},
    {Py_tp_traverse, Nil_traverse}, {Py_nb_bool, Nil_bool}, {0, NULL},
};

static PyType_Spec Nil_Type_Spec = {
    .name = "fastcons.nil",
    .basicsize = sizeof(NilObject),
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_IMMUTABLETYPE,
    .slots = Nil_Type_Slots,
};

/* The Cons type */

typedef struct {
    PyObject_HEAD PyObject *head;
    PyObject *tail;
    int list_len;
} ConsObject;

PyObject *
Cons_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    ConsObject *self = (ConsObject *)type->tp_alloc(type, 0);
    if (self == NULL) {
        return NULL;
    }

    static char *kwlist[] = {"head", "tail", NULL};
    PyObject *head = NULL, *tail = NULL;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "OO", kwlist, &head, &tail)) {
        DECREF_AND_NULLIFY(self);
        return NULL;
    }

    consmodule_state *state = PyType_GetModuleState(type);
    if (state == NULL)
        return NULL;
    PyObject *nil = state->nil;
    PyTypeObject *cons = (PyTypeObject *)state->ConsType;

    if (Py_Is(tail, nil)) {
        self->list_len = 1;
    }
    else if (Py_IS_TYPE(tail, cons)) {
        self->list_len = IS_LIST(tail) ? 1 + LIST_LEN(tail) : -1;
    }
    else {
        self->list_len = 0;
    }

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
    PyObject_GC_UnTrack(self);
    Py_TRASHCAN_BEGIN(self, Cons_dealloc);
    Cons_clear((PyObject *)self);
    Py_TYPE(self)->tp_free(self);
    Py_TRASHCAN_END;
}

PyObject *
Cons_from_fast(PyObject *xs, consmodule_state *state)
{
    PyObject *nil = state->nil;
    PyObject *cons = state->ConsType;

    Py_ssize_t len = PySequence_Fast_GET_SIZE(xs);
    PyObject *result = nil, *item;
    for (Py_ssize_t i = len - 1; i >= 0; i--) {
        item = PySequence_Fast_GET_ITEM(xs, i);
        Py_INCREF(item);
        result = PyObject_CallFunctionObjArgs(cons, item, result, NULL);
        DECREF_AND_NULLIFY(item);
        if (result == NULL)
            break;
    }

    return result;
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

    PyObject *xs = args[0], *tp = NULL, *result = NULL;

    if (PyGen_Check(xs)) {
        // We should iterate in reverse order, so need a concrete sequence.
        // Converting a generator to a tuple is not as fast as iterating directly over
        // the generator, but much faster than converting it to a list or relying on
        // PySequence_Fast.
        tp = PySequence_Tuple(xs);
        xs = tp;
    }

    if ((xs = PySequence_Fast(xs, "Expected a sequence or iterable")) != NULL) {
        result = Cons_from_fast(xs, state);
        DECREF_AND_NULLIFY(xs);
    }

    XDECREF_AND_NULLIFY(tp);
    return result;
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

    PyObject *list = PyList_New(LIST_LEN(self));
    PyObject *next = self, *head = NULL;
    for (Py_ssize_t i = 0; i < LIST_LEN(self); i++) {
        head = CAR(next);
        Py_INCREF(head);  // PyList_SET_ITEM steals a reference
        PyList_SET_ITEM(list, i, head);
        next = CDR(next);
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
    if (i != 0) {
        return i > 0 ? PyUnicode_FromFormat("...") : NULL;
    }

    _PyUnicodeWriter_Init(&writer);
    writer.overallocate = 1;
    writer.min_length = 3;  // "(_)"
    if (_PyUnicodeWriter_WriteChar(&writer, '(') < 0) {
        goto error;
    }

    PyObject *head = NULL, *repr = NULL, *tail = NULL;
    while (Py_IS_TYPE(next, cons)) {
        head = CAR(next);
        repr = PyObject_Repr(head);
        if (repr == NULL)
            goto error;
        if (_PyUnicodeWriter_WriteStr(&writer, repr) < 0) {
            DECREF_AND_NULLIFY(repr);
            goto error;
        }
        DECREF_AND_NULLIFY(repr);

        tail = CDR(next);
        if (Py_Is(tail, state->nil))
            break;
        else if (!Py_IS_TYPE(tail, cons)) {
            if (_PyUnicodeWriter_WriteASCIIString(&writer, " . ", 3) < 0) {
                goto error;
            }
            repr = PyObject_Repr(tail);
            if (repr == NULL) {
                goto error;
            }
            if (_PyUnicodeWriter_WriteStr(&writer, repr) < 0) {
                DECREF_AND_NULLIFY(repr);
                goto error;
            }
            DECREF_AND_NULLIFY(repr);
            break;
        }
        if (_PyUnicodeWriter_WriteChar(&writer, ' ') < 0) {
            goto error;
        }
        next = tail;
    }

    writer.overallocate = 0;
    if (_PyUnicodeWriter_WriteChar(&writer, ')') < 0) {
        goto error;
    }
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

    PyTypeObject *cons = (PyTypeObject *)state->ConsType;
    if (!Py_IS_TYPE(other, cons))
        Py_RETURN_FALSE;

    PyObject *this = self, *that = other;
    while (Py_IS_TYPE(this, cons) && Py_IS_TYPE(that, cons)) {
        switch (PyObject_RichCompareBool(CAR(this), CAR(that), op)) {
            case -1:
                return NULL;
            case 0:
                Py_RETURN_FALSE;
            case 1:
                this = CDR(this);
                that = CDR(that);
        }
    }
    return PyObject_RichCompare(this, that, op);
}

static PyMemberDef Cons_members[] = {
    {"head", T_OBJECT_EX, offsetof(ConsObject, head), READONLY, "cons head"},
    {"tail", T_OBJECT_EX, offsetof(ConsObject, tail), READONLY, "cons tail"},
    {NULL},
};

PyDoc_STRVAR(from_xs_doc, "Create a cons list from a sequence or iterable");
PyDoc_STRVAR(to_list_doc, "Convert a proper const list to a Python list");

static PyMethodDef Cons_methods[] = {
    {"from_xs", (PyCFunction)Cons_from_xs,
     METH_METHOD | METH_FASTCALL | METH_KEYWORDS | METH_CLASS, from_xs_doc},
    {"to_list", (PyCFunction)Cons_to_list, METH_METHOD | METH_FASTCALL | METH_KEYWORDS,
     to_list_doc},
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
    {0, NULL},
};

static PyType_Spec Cons_Type_Spec = {
    .name = "fastcons.cons",
    .basicsize = sizeof(ConsObject),
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_IMMUTABLETYPE | Py_TPFLAGS_HAVE_GC,
    .slots = Cons_Type_Slots,
};

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
        DECREF_AND_NULLIFY(match_args);
        return -1;
    }
    DECREF_AND_NULLIFY(match_args);

    state->NilType = PyType_FromModuleAndSpec(m, &Nil_Type_Spec, NULL);
    if (state->NilType == NULL)
        return -1;
    if (PyModule_AddType(m, (PyTypeObject *)state->NilType) < 0)
        return -1;

    PyObject *nil =
        ((PyTypeObject *)state->NilType)->tp_alloc((PyTypeObject *)state->NilType, 0);
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

static struct PyModuleDef consmodule = {
    PyModuleDef_HEAD_INIT,
    .m_name = "fastcons",
    .m_doc = "Module exporting cons type",
    .m_size = sizeof(consmodule_state),
    .m_methods = NULL,
    .m_slots = consmodule_slots,
    .m_traverse = consmodule_traverse,
    .m_clear = consmodule_clear,
};

PyMODINIT_FUNC
PyInit_fastcons(void)
{
    return PyModuleDef_Init(&consmodule);
}
