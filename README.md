# fastcons

Fastcons is a Python extension module that aims to provide an efficient implementation of cons.

The fastcons module provides two types: `nil` and `cons`. The `nil` type represents the empty list, while the cons type represents a pair - an immutable cell containing two elements.

Currently requires Python 3.11, and Linux or MacOS.

* [Installation](#installation)
* [Usage](#usage)
  + [Pattern matching](#pattern-matching)
* [API Reference](#api-reference)
* [License](#license)

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
xs = cons(1, cons(2, cons(3, nil())))

# Create a cons list from a Python sequence
ys = cons.from_xs([1, 2, 3])

# Access the head and tail of a cons list
assert xs.head == 1
assert xs.tail.head == 2

# Test for equality
assert xs == ys
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

`cons.lift` can be used to recursively transform dicts, lists, tuples and generators to `cons` objects. dicts will be transformed to cons lists of pairs (association lists or alists), the rest will be transformed to alists.

``` python-console
>>> cons.lift({'a': 2, 'b': 3})
(('a' . 2) ('b' . 3))
>>> cons.lift((1, 2, 3))
(1 2 3)
>>> cons.lift(x for x in ('a', 'b', 'c'))
('a' 'b' 'c')
>>> cons.lift({x: {x: y}} for x, y in zip(['a', 'b', 'c'], range(1, 4)))
((('a' ('a' . 1))) (('b' ('b' . 2))) (('c' ('c' . 3))))
```

The `nil` object provides a `to_list()` method that returns an empty Python list:

``` python-console
>>> nil().to_list()
[]
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

Returns the singleton `nil` object. The `nil` object is falsy.

### `nil.to_list()`

Returns an empty Python list.

### `cons(head, tail)`

Returns a `cons` object with the given `head` and `tail`.

### `cons.from_xs(xs)`

Returns a `cons` object created from the Python sequence `xs`.

### `cons.lift(xs)`

Recursively create a `cons` structure by converting:

- lists, tuples, and generators to `cons` lists; and
- dicts to `cons` lists of pairs (association lists).

### `assoc(object, alist)`

Find the first pair in `alist` whose car is equal to `object`, and return that pair. If no pair is found, or `alist` is `nil()`, return `nil()`.

### `assp(predicate, alist)`

Return the first pair in alist for which the result of calling 'predicate' on its car is truthy. 'predicate' will be called with a single positional argument.


## License

`fastcons` is released under the MIT license.
