#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <structmember.h>

static PyTypeObject ConsType;

typedef struct {
    PyObject_HEAD
} NilObject;

PyObject *
Nil_repr(PyObject *self)
{
    return PyUnicode_FromString("nil()");
}

static PyTypeObject NilType = {
    PyVarObject_HEAD_INIT(NULL, 0).tp_name = "cons.nil",
    .tp_doc = PyDoc_STR("The empty list"),
    .tp_basicsize = sizeof(NilObject),
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_IMMUTABLETYPE,
    .tp_new = PyType_GenericNew,
    .tp_repr = (reprfunc)&Nil_repr,
};

static PyObject *_Nil = NULL;

static PyObject *
nil(PyObject *self, PyObject *args)
{
    if (_Nil == NULL) {
        _Nil = PyObject_CallObject((PyObject *)&NilType, NULL);
    }
    Py_INCREF(_Nil);
    return _Nil;
}

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
        self->head = tail;
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

static PyMemberDef Cons_members[] = {
    {"head", T_OBJECT_EX, offsetof(ConsObject, head), READONLY, "cons head"},
    {"tail", T_OBJECT_EX, offsetof(ConsObject, tail), READONLY, "Cons tail"},
};

static PyTypeObject ConsType = {
    PyVarObject_HEAD_INIT(NULL, 0).tp_name = "cons.cons",
    .tp_doc = PyDoc_STR("Construct a new pair"),
    .tp_basicsize = sizeof(ConsObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_IMMUTABLETYPE | Py_TPFLAGS_HAVE_GC,
    .tp_dealloc = (destructor)Cons_dealloc,
    .tp_new = Cons_new,
    .tp_members = Cons_members,
    .tp_traverse = (traverseproc)Cons_traverse,
    .tp_clear = (inquiry)Cons_clear,
};

static PyMethodDef ConsModuleMethods[] = {
    {"nil", nil, METH_NOARGS, "Return the empty cons list"},
    {NULL, NULL, 0, NULL},
};

static PyModuleDef consmodule = {
    PyModuleDef_HEAD_INIT,
    .m_name = "cons",
    .m_doc = "Module exporting cons type",
    .m_size = -1,
    .m_methods = ConsModuleMethods,
};

PyMODINIT_FUNC
PyInit_cons(void)
{
    PyObject *m;
    if (PyType_Ready(&ConsType) < 0)
        return NULL;

    if (PyType_Ready(&NilType) < 0)
        return NULL;

    m = PyModule_Create(&consmodule);
    if (m == NULL)
        return NULL;

    Py_IncRef((PyObject *)&ConsType);
    if (PyModule_AddObject(m, "cons", (PyObject *)&ConsType) < 0) {
        Py_DECREF(&ConsType);
        Py_DECREF(m);
        return NULL;
    }
    return m;
}
