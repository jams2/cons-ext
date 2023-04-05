from collections.abc import Iterable
from typing import Self

class nil:
    pass

class cons:
    def __init__(self, head, any): ...
    def from_xs(self, xs: Iterable) -> Self | nil: ...
