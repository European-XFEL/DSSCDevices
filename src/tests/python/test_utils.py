import pytest
from DsscControl.utils import get_sweep_vector

@pytest.mark.parametrize(
    "user_input, expected_bool, expected_seq",
    [
        ("range(0, 2)", True, [0, 1]),
        ("[0, 1, 2]", True, [0, 1, 2]),
        ("ra", False, "Invalid Expression"),
        ("range", False, "Invalid Expression"),
        ("range(0, 2", False, "Invalid Expression"),
    ],
)
def test_sweep_vector(user_input, expected_bool, expected_seq):
    ok, sequence = get_sweep_vector(user_input)
    assert ok == expected_bool
    assert sequence == expected_seq
    

