#include <Python.h>
#include <structmember.h>

#include <stdlib.h>
#include <pthread.h>


typedef struct {
    PyObject_HEAD
    pthread_rwlock_t *rwlock;
} RWLockObject;

static PyObject* RWLockException;

static void
RWLock_del(RWLockObject *self)
{
    int result;
    result = pthread_rwlock_destroy(self->rwlock);
    if (result) {
        //raise some exception ?
    } else {
        free(self->rwlock);
    }
}

static int
RWLock_init(RWLockObject *self, PyObject *args, PyObject *kwds)
{

    static char *kwlist[] = {NULL};
    int result;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist))
        return -1;

    // TODO: handle attrs from init
    pthread_rwlockattr_t *attr = NULL;
    self->rwlock = malloc(sizeof(pthread_rwlock_t));
    result = pthread_rwlock_init(self->rwlock, attr);
    if (result) {
        return -1;
    }

    return 0;
}

static PyMemberDef RWLock_members[] = {
    {NULL}
};


static PyObject *
RWLock_operation(RWLockObject *self, PyObject *args, PyObject *kwargs)
{
    // rdlock, wrlock, unlock
    static char *kwlist[] = {"operation", "blocking", NULL};
    PyObject *blocking = Py_False;
    char operation;
    int error = 0;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "c|O", kwlist,
            &operation, &blocking)) {
        return NULL;
    }

    switch (operation) {
        case 'r':
            if (PyObject_IsTrue(blocking)) {
                error = pthread_rwlock_rdlock(self->rwlock);
            } else {
                error = pthread_rwlock_tryrdlock(self->rwlock);
                if (error == EBUSY) {
                    Py_RETURN_FALSE;
                }
            }
            break;
        case 'w':
            if (PyObject_IsTrue(blocking)) {
                error = pthread_rwlock_wrlock(self->rwlock);
            } else {
                error = pthread_rwlock_trywrlock(self->rwlock);
                if (error == EBUSY) {
                    Py_RETURN_FALSE;
                }
            }
            break;
        case 'u':
            error = pthread_rwlock_unlock(self->rwlock);
            break;
        default:
            PyErr_SetString(RWLockException, "Unknown operation");
            return NULL;
    }

    if (error) {
        PyErr_SetString(RWLockException, "RWLock exception");
        return NULL;
    }
    Py_RETURN_TRUE;
}

static PyObject *
RWLock_wrlock(RWLockObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = {"blocking", NULL};
    PyObject *blocking = Py_False;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|O", kwlist, &blocking)) {
        return NULL;
    }
    return RWLock_operation(self, args, \
                            Py_BuildValue("{s:s,s:O}", \
                                "operation", "w", \
                                "blocking", blocking));
}

static PyObject *
RWLock_rdlock(RWLockObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = {"blocking", NULL};
    PyObject *blocking = Py_False;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|O", kwlist, &blocking)) {
        return NULL;
    }
    return RWLock_operation(self, args, \
                            Py_BuildValue("{s:s,s:O}", \
                                "operation", "r", \
                                "blocking", blocking));
}

static PyObject *
RWLock_unlock(RWLockObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = {"blocking", NULL};
    PyObject *blocking = Py_False;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|O", kwlist, &blocking)) {
        return NULL;
    }
    return RWLock_operation(self, args, \
                            Py_BuildValue("{s:s,s:O}", \
                                "operation", "u", \
                                "blocking", blocking));
}

static PyMethodDef RWLock_methods[] = {
    {"wrlock", (PyCFunction)RWLock_wrlock, METH_VARARGS | METH_KEYWORDS, "Aquire write lock."},
    {"rdlock", (PyCFunction)RWLock_rdlock, METH_VARARGS | METH_KEYWORDS, "Aquire read lock."},
    {"unlock", (PyCFunction)RWLock_unlock, METH_VARARGS | METH_KEYWORDS, "Unlock previously aquired lock."},
    //{"operation", (PyCFunction)RWLock_operation, METH_VARARGS | METH_KEYWORDS, "Operation"},
    {NULL}
};

static char RWLock_doc[] = "pthread rwlock functions wrapper";

static PyTypeObject RWLockObjectType = {
    PyObject_HEAD_INIT(NULL)
    0,              /* ob_size        */
    "posix_rwlock.RWLock",        /* tp_name        */
    sizeof(RWLockObject),       /* tp_basicsize   */
    0,              /* tp_itemsize    */
    (destructor)RWLock_del,        /* tp_del */
    0,              /* tp_print       */
    0,              /* tp_getattr     */
    0,              /* tp_setattr     */
    0,              /* tp_compare     */
    0,              /* tp_repr        */
    0,              /* tp_as_number   */
    0,              /* tp_as_sequence */
    0,              /* tp_as_mapping  */
    0,              /* tp_hash        */
    0,              /* tp_call        */
    0,              /* tp_str         */
    0,              /* tp_getattro    */
    0,              /* tp_setattro    */
    0,              /* tp_as_buffer   */
    Py_TPFLAGS_DEFAULT,     /* tp_flags       */
    RWLock_doc,       /* tp_doc         */
    0,              /* tp_traverse       */
    0,              /* tp_clear          */
    0,              /* tp_richcompare    */
    0,              /* tp_weaklistoffset */
    0,              /* tp_iter           */
    0,              /* tp_iternext       */
    RWLock_methods,               /* tp_methods        */
    RWLock_members,           /* tp_members        */
    0,              /* tp_getset         */
    0,              /* tp_base           */
    0,              /* tp_dict           */
    0,              /* tp_descr_get      */
    0,              /* tp_descr_set      */
    0,              /* tp_dictoffset     */
    (initproc)RWLock_init,        /* tp_init           */
    0,              /* tp_alloc */
    0,              /* tp_new */
    0,              /* tp_free */
    0,              /* tp_is_gc */
    0,              /* tp_bases */
    0,              /* tp_mro */
    0,              /* tp_cache */
    0,              /* tp_subclasses */
    0,              /* tp_weaklist */
};

PyMODINIT_FUNC
initposix_rwlock(void)
{
    PyObject* m;

    RWLockObjectType.tp_new = PyType_GenericNew;
    if (PyType_Ready(&RWLockObjectType) < 0)
        return;

    m = Py_InitModule3("posix_rwlock", NULL,
               "Example module that creates an extension type.");
    if (m == NULL)
        return;

    RWLockException = PyErr_NewException("posix_rwlock.RWLockException", NULL, NULL);
    Py_INCREF(RWLockException);
    PyModule_AddObject(m, "RWLockException", RWLockException);

    Py_INCREF(&RWLockObjectType);
    PyModule_AddObject(m, "RWLock", (PyObject *)&RWLockObjectType);
}
