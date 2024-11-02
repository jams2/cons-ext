from collections.abc import Callable, Iterable
from typing import Any, Self

class nil:
    def to_list(self) -> list[Any]: ...

class cons:
    head: Any
    tail: Any

    def __init__(self, head: Any, tail: Any) -> None: ...
    def to_list(self) -> list[Any]: ...
    @classmethod
    def from_xs(cls, xs: Iterable[Any]) -> Self | nil: ...
    @classmethod
    def lift(cls, xs: Any) -> Any | Self | nil: ...

def assoc(object: Any, alist: cons | nil) -> cons | nil: ...
def assp(predicate: Callable[[Any], bool], alist: cons | nil) -> cons | nil: ...
