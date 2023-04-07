# fastcons

Fastcons is a Python extension module that provides an efficient implementation of cons lists.

The fastcons module provides two types: `nil` and `cons`. The `nil` type represents the empty list, while the cons type represents a pair - an immutable cell containing two elements.

Currently requires Python 3.11.

## Installation

You can install fastcons using pip:

``` python
pip install fastcons
```

## Usage

The fastcons module provides two types:

- `nil`: represents the empty list; and
- `cons`: represents a pair.

You can create the `nil` object by calling `nil()`.

You can create a `cons` object by calling `cons(head, tail)`. `cons` can be used to create linked lists: a chain of `cons` objects is considered a list if it is terminated by `nil()`, e.g. `cons(1, cons(2, cons(3, nil())))`.

You can efficiently create `cons` lists from Python sequences using the `cons.from_xs` method, where `xs` is a sequence.

``` python
from fastcons import cons, nil

# Create a cons list using the cons function
lst = cons(1, cons(2, cons(3, nil())))

# Create a cons list from a Python sequence
lst2 = cons.from_xs([1, 2, 3])

# Access the head and tail of a cons list
assert lst.head == 1
assert lst.tail.head == 2

# Test for equality
assert lst == lst2
```

The `cons` objects are printed using Lisp-style notation, which makes it easier to read long lists.

``` python-console
>>> cons.from_xs(range(1, 4))
(1 2 3)
>>> cons("foo", "bar")
('foo' . 'bar')
>>> cons(cons(1, 2), cons(cons(3, 4), nil()))
((1 . 2) (3 . 4))
```

### Pattern matching

PEP 622 pattern matching is supported for `cons` and `nil`:

``` python-console
>>> match nil():
...   case nil():
...     print(nil())
...
nil()
>>> match cons(1, nil()):
...   case cons(a, d):
...     print(f"{a = }, {d = }")
...
a = 1, d = nil()
```

## API Reference

### `nil()`

Returns the singleton `nil` object.

### `cons(head, tail)`

Returns a `cons` object with the given `head` and `tail`.

### `cons.from_xs(xs)`

Returns a `cons` object created from the Python sequence `xs`.

## License

`fastcons` is released under the MIT license.
