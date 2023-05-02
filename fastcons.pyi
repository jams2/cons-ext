from collections.abc import Iterable
from typing import Any, Self

class nil:
    pass

class cons:
    def __init__(self, head: Any, tail: Any): ...
    @classmethod
    def from_xs(cls, xs: Iterable) -> Self | nil: ...
    @classmethod
    def lift(cls, xs: Any) -> Any: ...

def assoc(object: Any, alist: cons) -> cons | nil: ...
