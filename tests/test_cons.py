import pytest
from fastcons import cons, nil


@pytest.mark.parametrize(
    "args",
    [
        (),
        (1,),
        (1, 2, 3),
    ],
)
def test_bad_nargs(args):
    with pytest.raises(TypeError):
        cons(*args)


def test_create():
    pair = cons(1, 2)
    assert pair.head == 1
    assert pair.tail == 2


@pytest.mark.parametrize(
    ("xs", "expected"),
    [
        # Check empty sequences evaluate to nil()
        ([], nil()),
        ("", nil()),
        ((), nil()),
        (range(0), nil()),
        (set(), nil()),
        ({}, nil()),
        # Check single-element sequences evaluate to a pair with nil() as the tail
        ([1], cons(1, nil())),
        ("1", cons("1", nil())),
        ((1,), cons(1, nil())),
        (range(1), cons(0, nil())),
        ({1}, cons(1, nil())),
        ({1: 2}, cons(1, nil())),
        # Check multi-element sequences evaluate to a pair with a pair as the tail
        ([1, 2], cons(1, cons(2, nil()))),
        ("12", cons("1", cons("2", nil()))),
        ((1, 2), cons(1, cons(2, nil()))),
        (range(2), cons(0, cons(1, nil()))),
        ({1, 2}, cons(1, cons(2, nil()))),
        ({1: 2, 3: 4}, cons(1, cons(3, nil()))),
    ],
)
def test_from_xs(xs, expected):
    pair = cons.from_xs(xs)
    assert pair == expected


@pytest.mark.parametrize(
    ("obj", "expected"),
    [
        (cons(1, 2), "(1 . 2)"),
        (cons(1, nil()), "(1)"),
        (cons(1, cons(2, nil())), "(1 2)"),
        (cons(1, cons(2, cons(3, nil()))), "(1 2 3)"),
        (cons(1, cons(2, cons(3, 4))), "(1 2 3 . 4)"),
        # Mix it up with some different types
        (cons(1, "2"), "(1 . '2')"),
        (cons(1, 2.0), "(1 . 2.0)"),
        (cons(1, True), "(1 . True)"),
    ],
)
def test_cons_repr(obj, expected):
    assert repr(obj) == expected


def test_repr_with_cycle():
    x = ["x"]
    p = cons(1, cons(x, nil()))
    x.append(p)
    assert repr(p) == "(1 ['x', ...])"


def test_repr_with_multiple_cycles():
    x = ["x"]
    y = ["y"]
    p = cons(y, nil())
    q = cons(1, cons(x, p))
    x.append(p)
    y.append(q)
    assert repr(q) == "(1 ['x', (['y', ...])] ['y', ...])"
