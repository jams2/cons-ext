import sys

import pytest
from fastcons import nil


def test_nil_takes_no_args():
    with pytest.raises(TypeError):
        nil(1)


def test_nil_is_nil():
    assert nil() is nil()


def test_nil_eq_nil():
    assert nil() == nil()


@pytest.mark.xfail()
def test_identity_preserved_across_imports():
    sys.modules.pop("fastcons")
    import fastcons

    x = nil()
    sys.modules.pop("fastcons")
    import fastcons as fastcons_2

    assert x is fastcons_2.nil()


def test_hashable():
    d = {nil(): "hello"}
    assert nil() in d
