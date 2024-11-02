import gc
import weakref

import pytest
from fastcons import nil


def test_nil_takes_no_args():
    with pytest.raises(TypeError):
        nil(1)  # type: ignore


def test_nil_is_nil():
    assert nil() is nil()


def test_nil_eq_nil():
    assert nil() == nil()


def test_nil_bool():
    """Test nil() is falsey."""
    assert not nil()
    assert bool(nil()) is False


def test_hashable():
    d = {nil(): "hello"}
    assert nil() in d


def test_nil_to_list():
    """Test nil().to_list() returns an empty list."""
    assert nil().to_list() == []


def test_nil_to_list_new_object():
    """Test nil().to_list() returns a new list each time."""
    lst1 = nil().to_list()
    lst2 = nil().to_list()
    assert lst1 == lst2
    assert lst1 is not lst2


def test_weakref():
    """
    nil supports weakrefs.

    Although not particularly useful, as it's a singleton object, it
    should be part of the interface for common behaviour with cons.
    """
    x = nil()
    ref = weakref.ref(x)
    assert ref() is nil()
    del x
    gc.collect()
    assert nil() is nil()
