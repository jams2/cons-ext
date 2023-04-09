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

typedef struct {
    PyObject_HEAD
} NilObject;

typedef struct {
    PyObject_HEAD PyObject *head;
    PyObject *tail;
    unsigned long list_len;
} ConsObject;

typedef struct {
    PyObject *NilType;
    PyObject *nil;
    PyObject *ConsType;
    ConsObject *free;
} consmodule_state;

static ConsObject *
maybe_freelist_pop(consmodule_state *);
static void
freelist_push(consmodule_state *, ConsObject *);
static void
freelist_clear(consmodule_state *);
static struct PyModuleDef consmodule;
