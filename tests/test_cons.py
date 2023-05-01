import operator
from collections import namedtuple

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


@pytest.mark.parametrize(
    ("op", "a", "b", "expected"),
    [
        # eq
        (operator.eq, cons(1, 2), cons(1, 2), True),
        (operator.eq, cons(1, 2), cons(1, 3), False),
        (operator.eq, cons(1, 2), cons(2, 2), False),
        (operator.eq, cons.from_xs([1, 2]), cons.from_xs([1, 2]), True),
        (operator.eq, cons.from_xs([1, 2]), cons.from_xs([1, 3]), False),
        (operator.eq, cons.from_xs([1, 2]), cons.from_xs([2, 2]), False),
        (operator.eq, cons.from_xs([1, 2]), cons.from_xs([1, 2, 3]), False),
        (operator.eq, cons.from_xs([1, 2]), cons.from_xs([1, 1, 3]), False),
        (operator.eq, cons.from_xs(range(100)), cons.from_xs(range(100)), True),
        (operator.eq, cons.from_xs(range(100)), cons.from_xs(range(1, 101)), False),
        (operator.eq, cons.from_xs(range(100)), cons.from_xs(range(99)), False),
        (operator.eq, cons(1, cons(2, 3)), cons(1, cons(2, 3)), True),
        (operator.eq, cons(1, cons(2, 3)), cons(1, cons(2, 4)), False),
        # ne
        (operator.ne, cons(1, 2), cons(1, 2), False),
        (operator.ne, cons(1, 2), cons(1, 3), True),
        (operator.ne, cons.from_xs([1, 2]), cons.from_xs([1, 2]), False),
        (operator.ne, cons.from_xs([1, 2]), cons.from_xs([1, 3]), True),
        (operator.ne, cons.from_xs([1, 2]), cons.from_xs([2, 2]), True),
        (operator.ne, cons(1, cons(2, 3)), cons(1, cons(2, 3)), False),
        (operator.ne, cons(1, cons(2, 3)), cons(1, cons(2, 4)), True),
        (operator.ne, cons.from_xs([1, 2]), cons.from_xs([1, 2, 3]), True),
        (operator.ne, cons.from_xs([1, 2]), cons.from_xs([1, 1, 3]), True),
        # gt
        (operator.gt, cons(1, 1), cons(0, 0), True),
        (operator.gt, cons(2, cons(2, cons(2, 2))), cons(1, cons(1, cons(1, 1))), True),
        (operator.gt, cons(2, (2,)), cons(1, (1,)), True),
        (operator.gt, cons((2,), (2,)), cons((1,), (1,)), True),
        # lt
        (operator.lt, cons(1, 1), cons(0, 0), False),
        (
            operator.lt,
            cons(2, cons(2, cons(2, 2))),
            cons(1, cons(1, cons(1, 1))),
            False,
        ),
        (operator.lt, cons(2, (2,)), cons(1, (1,)), False),
        (operator.lt, cons((2,), (2,)), cons((1,), (1,)), False),
        # ge
        (operator.ge, cons(1, 1), cons(0, 0), True),
        (operator.ge, cons(2, 2), cons(1, 1), True),
        (operator.ge, cons(2, 2), cons(2, 2), True),
        (operator.ge, cons(2, 2), cons(3, 3), False),
        (operator.ge, cons(2, (2,)), cons(1, (1,)), True),
        (operator.ge, cons((2,), (2,)), cons((1,), (1,)), True),
        # le
        (operator.le, cons(1, 1), cons(0, 0), False),
        (operator.le, cons(2, 2), cons(1, 1), False),
    ],
)
def test_cons_richcompare(op, a, b, expected):
    assert op(a, b) == expected


def test_cons_to_list():
    cons_list = cons.from_xs(range(100))
    assert cons_list.to_list() == list(range(100))


def test_cons_hashable():
    d = {cons(1, 2): "hello"}
    assert cons(1, 2) in d


def test_hash_equality():
    a = hash(cons(1, cons(2, cons(3, nil()))))
    b = hash(cons(1, cons(2, cons(3, nil()))))
    assert a == b


def test_not_hashable_when_members_not_hashable():
    with pytest.raises(TypeError):
        hash(cons([1], nil()))


@pytest.mark.parametrize(
    ("xs", "expected"),
    [
        ({1: 2, 3: 4, 5: 6}, "((1 . 2) (3 . 4) (5 . 6))"),
        ({"a": {1: 2}, "b": {3: 4}}, "(('a' (1 . 2)) ('b' (3 . 4)))"),
    ],
)
def test_lift_dict(xs, expected):
    assert repr(cons.lift(xs)) == expected


@pytest.mark.parametrize("x", ["foo", b"foo", 123, 123.0])
def test_lift_atomics(x):
    assert cons.lift(x) == x


@pytest.mark.parametrize(
    "sequence",
    [
        (),
        {},
        [],
        (x for x in []),
    ],
)
def test_lift_empty_sequences(sequence):
    assert cons.lift(sequence) == nil()


@pytest.mark.parametrize(
    ("xs", "expected"),
    [
        ((x for x in range(10)), "(0 1 2 3 4 5 6 7 8 9)"),
        (
            ((x, y) for x, y in zip(range(7), range(7))),
            "((0 0) (1 1) (2 2) (3 3) (4 4) (5 5) (6 6))",
        ),
        (((x for x in range(2)) for _ in range(3)), "((0 1) (0 1) (0 1))"),
    ],
)
def test_lift_generator(xs, expected):
    assert repr(cons.lift(xs)) == expected


def test_lift_namedtuple():
    Foo = namedtuple("Foo", "foo bar baz")
    assert repr(cons.lift(Foo("a", "b", "c"))) == "('a' 'b' 'c')"


@pytest.mark.parametrize(
    ("xs", "expected"),
    [
        ([1, 2, 3], "(1 2 3)"),
        ([["a"], ["b"], ["c"]], "(('a') ('b') ('c'))"),
        ([[["a"]], [["b"]], [["c"]]], "((('a')) (('b')) (('c')))"),
    ],
)
def test_lift_list(xs, expected):
    assert repr(cons.lift(xs)) == expected


@pytest.mark.parametrize(
    ("xs", "expected"),
    [
        ((1, 2, 3), "(1 2 3)"),
        ((("a",), ("b",), ("c",)), "(('a') ('b') ('c'))"),
        (((("a",),), (("b",),), (("c",),)), "((('a')) (('b')) (('c')))"),
    ],
)
def test_lift_tuples(xs, expected):
    assert repr(cons.lift(xs)) == expected


@pytest.mark.parametrize(
    ("xs", "expected"),
    [
        (
            [
                {"a": [1, 2, 3], "b": [4, 5, 6]},
                {"c": [7, 8, 9], "d": [10, 11, 12]},
            ],
            "((('a' 1 2 3) ('b' 4 5 6)) (('c' 7 8 9) ('d' 10 11 12)))",
        ),
        (
            [
                {"a": (1, 2, 3), "b": (4, 5, 6)},
                {"c": (7, 8, 9), "d": (10, 11, 12)},
            ],
            "((('a' 1 2 3) ('b' 4 5 6)) (('c' 7 8 9) ('d' 10 11 12)))",
        ),
        (
            [
                {"a": (x for x in range(3)), "b": (x for x in range(3))},
                {"c": (x for x in range(3)), "d": (x for x in range(3))},
            ],
            "((('a' 0 1 2) ('b' 0 1 2)) (('c' 0 1 2) ('d' 0 1 2)))",
        ),
        (
            [
                {"a": list(range(3)), "b": list(range(3))},
                {"c": list(range(3)), "d": list(range(3))},
            ],
            "((('a' 0 1 2) ('b' 0 1 2)) (('c' 0 1 2) ('d' 0 1 2)))",
        ),
        (
            {
                "a": [1, 2, 3],
                "b": (4, 5, 6),
                "c": (x for x in range(3)),
                "d": list(range(3)),
            },
            "(('a' 1 2 3) ('b' 4 5 6) ('c' 0 1 2) ('d' 0 1 2))",
        ),
        (
            (
                [1, 2, 3],
                (4, 5, 6),
                (x for x in range(3)),
                list(range(3)),
            ),
            "((1 2 3) (4 5 6) (0 1 2) (0 1 2))",
        ),
        (
            (
                (x for x in range(3)),
                [1, 2, 3],
                (4, 5, 6),
                list(range(3)),
            ),
            "((0 1 2) (1 2 3) (4 5 6) (0 1 2))",
        ),
        (
            [
                {"a": [1, 2, 3], "b": [4, 5, 6]},
                {"c": [7, 8, 9], "d": [10, 11, 12]},
                [
                    {"e": [13, 14, 15], "f": [16, 17, 18]},
                    {"g": [19, 20, 21], "h": [22, 23, 24]},
                ],
                (
                    {"i": [25, 26, 27], "j": [28, 29, 30]},
                    {"k": [31, 32, 33], "l": [34, 35, 36]},
                ),
            ],
            "((('a' 1 2 3) ('b' 4 5 6)) (('c' 7 8 9) ('d' 10 11 12)) "
            "((('e' 13 14 15) ('f' 16 17 18)) (('g' 19 20 21) ('h' 22 23 24))) "
            "((('i' 25 26 27) ('j' 28 29 30)) (('k' 31 32 33) ('l' 34 35 36))))",
        ),
    ],
)
def test_lift_nested(xs, expected):
    assert repr(cons.lift(xs)) == expected
