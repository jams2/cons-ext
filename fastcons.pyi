from collections.abc import Iterable
from typing import Self

class nil:
    pass

class cons:
    def __init__(self, head, any): ...
    @classmethod
    def from_xs(cls, xs: Iterable) -> Self | nil: ...
