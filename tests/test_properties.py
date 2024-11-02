import gc
import weakref
from typing import cast

from fastcons import assoc, assp, cons, nil
from hypothesis import given
from hypothesis import strategies as st
from hypothesis.stateful import RuleBasedStateMachine, rule


@given(st.integers())
def test_cons_basic(x: int):
    """Test basic cons cell creation and access."""
    c = cons(x, nil())
    assert c.head == x
    assert c.tail is nil()


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
    if isinstance(data, list | dict):
        assert isinstance(lifted, cons) or lifted is nil()
    else:
        assert lifted == data


@given(st.lists(st.integers(), min_size=500), st.integers(min_value=1, max_value=100))
def test_cons_large_list(xs, n):
    """Test handling of large lists."""
    xs *= n
    c = cons.from_xs(xs)
    assert c.to_list() == xs


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

    if isinstance(result, nil):
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

    if isinstance(result, nil):
        assert all(k != target for k, _ in pairs)
    else:
        assert result.head == target
        idx = next(i for i, (k, _) in enumerate(pairs) if k == target)
        assert result.tail == pairs[idx][1]


@given(st.lists(st.integers()))
def test_from_xs_gc(lst):
    # Create some garbage first to increase GC pressure
    garbage = [cons(i, nil()) for i in range(1000)]
    result = cons.from_xs(lst)
    del garbage
    gc.collect()
    # Convert back to list to verify
    assert result.to_list() == lst


# State machine for testing cons list operations
class ConsListMachine(RuleBasedStateMachine):
    def __init__(self):
        super().__init__()
        self.py_list = []
        self.cons_list = nil()

    @rule(x=st.integers())
    def prepend(self, x):
        """
        Prepend to both lists and verify equality.
        """
        self.py_list = [x] + self.py_list
        self.cons_list = cons(x, self.cons_list)
        assert self.py_list == self.cons_list.to_list()

    @rule(idx=st.integers(min_value=0))
    def get_item(self, idx):
        """Test random access to elements."""
        if isinstance(self.cons_list, nil):
            return

        idx = idx % len(self.py_list)  # Wrap index

        # Get from cons list
        current = self.cons_list
        for _ in range(idx):
            current = current.tail

        assert current.head == self.py_list[idx]

    @rule()
    def verify_immutable(self):
        """Verify cons list hasn't been mutated unexpectedly."""
        old_list = self.cons_list.to_list()
        gc.collect()  # Try to trigger any potential memory issues
        assert self.cons_list.to_list() == old_list


test_cons_list_state = ConsListMachine.TestCase


# __repr__ tests
@given(st.text())
def test_repr_with_string_content(s):
    """Test repr handles arbitrary string content safely."""
    c = cons(s, nil())
    r = repr(c)
    assert isinstance(r, str)
    assert r.startswith("(")
    assert r.endswith(")")


@given(st.binary(min_size=0, max_size=1000))
def test_repr_with_bytes(b):
    """Test repr handles arbitrary bytes safely."""
    c = cons(b, nil())
    r = repr(c)
    assert isinstance(r, str)


@given(st.builds(cons.from_xs, st.lists(st.lists(st.text()), min_size=1)))
def test_repr_recursive_lists(xs):
    """Test repr handles nested structures."""
    r = repr(xs)
    assert isinstance(r, str)
    assert r.startswith("(")
    assert r.endswith(")")


@given(st.text(min_size=1000, max_size=10000))
def test_repr_very_long_string(s):
    """Test repr handles very long string content."""
    c = cons(s, nil())
    r = repr(c)
    assert isinstance(r, str)
    assert len(r) > 0


@given(st.lists(st.binary(min_size=0, max_size=1000), max_size=100))
def test_repr_binary_list(xs):
    """Test repr with lists of binary data."""
    c = cons.from_xs(xs)
    r = repr(c)
    assert isinstance(r, str)
    assert len(r) > 0
