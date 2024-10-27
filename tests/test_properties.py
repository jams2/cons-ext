from hypothesis import given, strategies as st
from hypothesis.stateful import RuleBasedStateMachine, rule

from fastcons import assoc, assp, cons, nil


# Custom strategies
@st.composite
def proper_cons_lists(draw, min_size=0, max_size=100):
    """
    Generate proper cons lists (those ending in nil()).
    """
    items = draw(st.lists(st.integers(), min_size=min_size, max_size=max_size))
    return cons.from_xs(items)


@st.composite
def improper_cons_pairs(draw):
    """
    Generate improper cons pairs (not ending in nil()).
    """
    head = draw(st.integers())
    tail = draw(st.integers())
    return cons(head, tail)


# Basic properties
@given(st.lists(st.integers()))
def test_to_list_roundtrip(xs):
    """
    Converting list->cons->list preserves values.
    """
    assert cons.from_xs(xs).to_list() == xs


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
    assert (xs == ys) == (ys == xs)


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
