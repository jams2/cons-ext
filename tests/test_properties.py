from typing import cast

from hypothesis import given, strategies as st, assume
from hypothesis.stateful import RuleBasedStateMachine, rule

from fastcons import assoc, assp, cons, nil


# Complex nested data structures
@st.composite
def nested_data(draw, max_depth=3):
    """
    Generate nested data structures including lists, tuples, dicts with varied types.
    """
    if max_depth == 0:
        return draw(st.one_of(st.none(), st.booleans(), st.integers(), st.text()))

    return draw(
        st.one_of(
            st.tuples(
                nested_data(max_depth=max_depth - 1),
                nested_data(max_depth=max_depth - 1),
            ),
            st.lists(nested_data(max_depth=max_depth - 1), min_size=1),
            st.dictionaries(
                st.text(min_size=1),
                nested_data(max_depth=max_depth - 1),
                min_size=1,
                max_size=5,
            ),
        )
    )


@given(nested_data())
def test_lift_preserves_structure(data):
    """
    Test that lift preserves the structure and values of nested data.
    """
    lifted = cons.lift(data)
    if isinstance(data, list | tuple | dict):
        assert lifted != nil()
        assert isinstance(lifted, cons)
    else:
        assert lifted == data


@given(st.lists(st.integers(), min_size=1))
def test_lift_list_roundtrip(xs):
    """
    Test that lifting a list and converting back preserves values.
    """
    lifted = cons.lift(xs)
    assert isinstance(lifted, cons)
    assert lifted.to_list() == xs


@given(st.dictionaries(st.text(), st.integers(), min_size=1))
def test_lift_dict_structure(d):
    """
    Test that lifting a dictionary creates proper association list structure.
    """
    lifted = cons.lift(d)
    assert isinstance(lifted, cons)

    # Check each pair in the association list
    current = lifted
    while current != nil():
        pair = current.head
        assert isinstance(pair, cons)
        assert pair.head in d
        assert pair.tail == d[pair.head]
        current = current.tail


@given(
    st.recursive(
        st.one_of(st.integers(), st.text(), st.booleans(), st.none()),
        lambda children: st.lists(children) | st.dictionaries(st.text(), children),
        max_leaves=10,
    )
)
def test_lift_recursive_structures(data):
    """
    Test that lift handles deeply nested recursive structures.
    """
    lifted = cons.lift(data)
    if isinstance(data, (list, dict)):
        assert isinstance(lifted, cons) or lifted is nil()
    else:
        assert lifted == data


@given(st.lists(st.integers()).map(lambda x: (x, (y for y in x))))
def test_lift_generators(data):
    """
    Test that lift properly handles generator expressions.
    """
    orig, gen = data
    lifted_list = cons.lift(orig)
    lifted_gen = cons.lift(gen)
    assert lifted_list == lifted_gen


# Custom strategies
@st.composite
def proper_cons_lists(draw, min_size=0):
    """
    Generate proper cons lists (those ending in nil()).
    """
    # Mix of integers, strings, bools, and None
    items = draw(
        st.lists(
            st.one_of(st.integers(), st.text(min_size=1), st.booleans(), st.none()),
            min_size=min_size,
        )
    )
    return cons.from_xs(items)


@st.composite
def improper_cons_pairs(draw):
    """
    Generate improper cons pairs (not ending in nil()).
    """
    # Mix of different types for both head and tail
    head = draw(st.one_of(st.integers(), st.text(min_size=1), st.booleans(), st.none()))
    tail = draw(st.one_of(st.integers(), st.text(min_size=1), st.booleans(), st.none()))
    return cons(head, tail)


# Basic properties
@given(st.lists(st.integers(), min_size=1))
def test_to_list_roundtrip(xs):
    """
    Converting list->cons->list preserves values.
    """

    assert cast(cons, cons.from_xs(xs)).to_list() == xs


@given(proper_cons_lists())
def test_cons_list_equality_reflexive(xs):
    """
    Cons list equality is reflexive.
    """
    assert xs == xs


@given(proper_cons_lists(), proper_cons_lists())
def test_cons_list_equality_symmetric(xs, ys):
    """
    Cons list equality is symmetric.
    """
    assert (xs == ys) is (ys == xs)


@given(proper_cons_lists(), proper_cons_lists(), proper_cons_lists())
def test_cons_list_equality_transitive(xs, ys, zs):
    """
    Cons list equality is transitive.
    """
    if xs == ys and ys == zs:
        assert xs == zs


@given(st.integers(), st.integers())
def test_cons_pair_attributes(head, tail):
    """
    Cons pairs preserve head/tail values.
    """
    pair = cons(head, tail)
    assert pair.head == head
    assert pair.tail == tail


# Association list properties
@given(st.integers(), st.lists(st.tuples(st.integers(), st.integers())))
def test_assoc_finds_first_match(key, pairs):
    """
    assoc finds the first matching pair.
    """
    alist = cons.from_xs([cons(k, v) for k, v in pairs])
    result = assoc(key, alist)

    if result is nil():
        assert all(k != key for k, _ in pairs)
    else:
        assert result.head == key
        idx = next(i for i, (k, _) in enumerate(pairs) if k == key)
        assert result.tail == pairs[idx][1]


@given(st.integers(), st.lists(st.tuples(st.integers(), st.integers())))
def test_assp_finds_first_match(target, pairs):
    """
    assp finds the first pair matching predicate.
    """
    alist = cons.from_xs([cons(k, v) for k, v in pairs])
    result = assp(lambda x: x == target, alist)

    if result is nil():
        assert all(k != target for k, _ in pairs)
    else:
        assert result.head == target
        idx = next(i for i, (k, _) in enumerate(pairs) if k == target)
        assert result.tail == pairs[idx][1]


# State machine for testing cons list operations
class ConsListMachine(RuleBasedStateMachine):
    def __init__(self):
        super().__init__()
        self.py_list = []
        self.cons_list = nil()

    @rule(x=st.integers())
    def append(self, x):
        """
        Append to both lists and verify equality.
        """
        self.py_list.append(x)
        self.cons_list = cons(x, self.cons_list)
        assert list(reversed(self.py_list)) == self.cons_list.to_list()


test_cons_list_state = ConsListMachine.TestCase
