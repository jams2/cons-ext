#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <structmember.h>

static struct PyModuleDef consmodule;

typedef struct {
    PyObject *NilType;
    PyObject *nil;
    PyObject *ConsType;
} consmodule_state;

typedef struct {
    PyObject_HEAD
} NilObject;

PyObject *
Nil_repr(PyObject *self)
{
    return PyUnicode_FromString("nil()");
}

static PyObject *
Nil_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    // Throw an error if any args or kwds passed
    if (PyTuple_Size(args) || (kwds && PyDict_Size(kwds))) {
        PyErr_SetString(PyExc_TypeError, "nil() takes no arguments");
        return NULL;
    }

    consmodule_state *state = PyType_GetModuleState(type);
    if (state == NULL) {
        return NULL;
    }
    PyObject *self = state->nil;
    Py_INCREF(self);
    return self;
}

PyDoc_STRVAR(Nil_doc, "Creates or returns the singleton nil object.");

static PyType_Slot Nil_Type_Slots[] = {
    {Py_tp_doc, (void *)Nil_doc},
    {Py_tp_new, Nil_new},
    {Py_tp_repr, Nil_repr},
    {0, NULL},
};

static PyType_Spec Nil_Type_Spec = {
    .name = "cons.nil",
    .basicsize = sizeof(NilObject),
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_IMMUTABLETYPE,
    .slots = Nil_Type_Slots,
};

/* The Cons type */

typedef struct {
    PyObject_HEAD PyObject *head;
    PyObject *tail;
} ConsObject;

PyObject *
Cons_new(PyTypeObject *subtype, PyObject *args, PyObject *kwds)
{
    ConsObject *self = (ConsObject *)subtype->tp_alloc(subtype, 0);
    if (self == NULL) {
        return NULL;
    }

    static char *kwlist[] = {"head", "tail", NULL};
    PyObject *head = NULL, *tail = NULL;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "OO", kwlist, &head, &tail))
        return NULL;

    if (head) {
        Py_INCREF(head);
        self->head = head;
    }

    if (tail) {
        Py_INCREF(tail);
        self->tail = tail;
    }

    return (PyObject *)self;
};

static int
Cons_traverse(ConsObject *self, visitproc visit, void *arg)
{
    Py_VISIT(self->head);
    Py_VISIT(self->tail);
    return 0;
}

int
Cons_clear(PyObject *self)
{
    Py_CLEAR(((ConsObject *)self)->head);
    Py_CLEAR(((ConsObject *)self)->tail);
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
Cons_from_list(PyObject *self, PyTypeObject *defining_class, PyObject *const *args,
               Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *list = args[0];
    if (!PyList_Check(list)) {
        PyErr_SetString(PyExc_TypeError, "Expected a list");
        return NULL;
    }

    consmodule_state *state = PyType_GetModuleState(defining_class);
    if (state == NULL) {
        return NULL;
    }

    PyObject *cons = state->ConsType, *result = state->nil, *item;
    Py_INCREF(state->nil);
    Py_INCREF(cons);
    for (Py_ssize_t i = PyList_GET_SIZE(list) - 1; i >= 0; i--) {
        item = PyList_GET_ITEM(list, i);
        Py_INCREF(item);
        result = PyObject_CallFunctionObjArgs(cons, item, result, NULL);
        Py_DECREF(item);
    }
    Py_DECREF(cons);
    return result;
}

PyObject *
Cons_from_iter(PyObject *self, PyTypeObject *defining_class, PyObject *const *args,
               Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *iter = NULL, *tmp = NULL;
    tmp = args[0];
    if ((iter = PyObject_GetIter(tmp)) == NULL) {
        return NULL;
    }

    consmodule_state *state = PyType_GetModuleState(defining_class);
    if (state == NULL) {
        return NULL;
    }

    PyObject *cons = state->ConsType, *result = state->nil, *item;
    Py_INCREF(state->nil);
    Py_INCREF(cons);
    while ((item = PyIter_Next(iter))) {
        result = PyObject_CallFunctionObjArgs(cons, item, result, NULL);
        Py_DECREF(item);
    }
    Py_DECREF(cons);
    return result;
}

static PyMemberDef Cons_members[] = {
    {"head", T_OBJECT_EX, offsetof(ConsObject, head), READONLY, "cons head"},
    {"tail", T_OBJECT_EX, offsetof(ConsObject, tail), READONLY, "cons tail"},
    {NULL},
};

static PyMethodDef Cons_methods[] = {
    {"from_list", (PyCFunction)Cons_from_list,
     METH_METHOD | METH_FASTCALL | METH_KEYWORDS | METH_CLASS,
     "Create a cons list from a list"},
    {"from_iter", (PyCFunction)Cons_from_iter,
     METH_METHOD | METH_FASTCALL | METH_KEYWORDS | METH_CLASS,
     "Create a cons list from an iterable"},
    {NULL},
};

PyDoc_STRVAR(cons_doc, "Construct a new immutable pair");

static PyType_Slot Cons_Type_Slots[] = {
    {Py_tp_doc, (void *)cons_doc},   {Py_tp_dealloc, Cons_dealloc},
    {Py_tp_new, Cons_new},           {Py_tp_members, Cons_members},
    {Py_tp_traverse, Cons_traverse}, {Py_tp_clear, Cons_clear},
    {Py_tp_methods, Cons_methods},   {0, NULL},
};

static PyType_Spec Cons_Type_Spec = {
    .name = "cons.cons",
    .basicsize = sizeof(ConsObject),
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_IMMUTABLETYPE | Py_TPFLAGS_HAVE_GC,
    .slots = Cons_Type_Slots,
};

static int
consmodule_exec(PyObject *m)
{
    consmodule_state *state = PyModule_GetState(m);
    if (state == NULL) {
        return -1;
    }

    state->ConsType = PyType_FromModuleAndSpec(m, &Cons_Type_Spec, NULL);
    if (state->ConsType == NULL) {
        return -1;
    }
    if (PyModule_AddType(m, (PyTypeObject *)state->ConsType) < 0) {
        return -1;
    }

    state->NilType = PyType_FromModuleAndSpec(m, &Nil_Type_Spec, NULL);
    if (state->NilType == NULL) {
        return -1;
    }
    if (PyModule_AddType(m, (PyTypeObject *)state->NilType) < 0) {
        return -1;
    }

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
    return 0;
}

static int
consmodule_clear(PyObject *m)
{
    consmodule_state *state = PyModule_GetState(m);
    Py_CLEAR(state->ConsType);
    Py_CLEAR(state->NilType);
    return 0;
}

static struct PyModuleDef consmodule = {
    PyModuleDef_HEAD_INIT,
    .m_name = "cons",
    .m_doc = "Module exporting cons type",
    .m_size = sizeof(consmodule_state),
    .m_methods = NULL,
    .m_slots = consmodule_slots,
    .m_traverse = consmodule_traverse,
    .m_clear = consmodule_clear,
};

PyMODINIT_FUNC
PyInit_cons(void)
{
    return PyModuleDef_Init(&consmodule);
}
