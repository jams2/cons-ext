import pytest
from fastcons import assoc, assp, cons, nil


@pytest.mark.parametrize(
    ("x", "xs", "expected"),
    [
        ("foo", nil(), nil()),
        ("foo", cons.lift({"foo": "bar", "baz": "quux"}), cons("foo", "bar")),
        (
            # assoc returns the *first* satisfying pair
            "baz",
            cons.from_xs([cons("foo", "bar"), cons("baz", "quux"), cons("baz", 42)]),
            cons("baz", "quux"),
        ),
        ("not found", cons.lift({"foo": "bar", "baz": "quux"}), nil()),
    ],
)
def test_assoc(x, xs, expected):
    assert assoc(x, xs) == expected


@pytest.mark.parametrize(
    ("predicate", "xs", "expected"),
    [
        (lambda x: x == "foo", nil(), nil()),
        (
            lambda x, *_: x == "foo",
            cons.lift({"foo": "bar", "baz": "quux"}),
            cons("foo", "bar"),
        ),
        (
            lambda x, *_, **__: x == "foo",
            cons.lift({"foo": "bar", "baz": "quux"}),
            cons("foo", "bar"),
        ),
        (
            lambda x, **__: x == "foo",
            cons.lift({"foo": "bar", "baz": "quux"}),
            cons("foo", "bar"),
        ),
        (
            lambda x: x == "foo",
            cons.lift({"foo": "bar", "baz": "quux"}),
            cons("foo", "bar"),
        ),
        (
            # assp returns the *first* satisfying pair
            lambda x: x == "baz",
            cons.from_xs([cons("foo", "bar"), cons("baz", "quux"), cons("baz", 42)]),
            cons("baz", "quux"),
        ),
        (lambda x: x == "not found", cons.lift({"foo": "bar", "baz": "quux"}), nil()),
    ],
)
def test_assp(predicate, xs, expected):
    assert assp(predicate, xs) == expected


def test_assp_predicate_kwonly_raises():
    with pytest.raises(TypeError):
        assp(lambda *, x: x is x, cons.lift({"foo": "bar", "baz": "quux"}))
