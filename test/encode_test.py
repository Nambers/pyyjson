import os
import sys
import unittest


class TestEncode(unittest.TestCase):
    def _check_obj_same(self, a, b):
        from test_utils import check_obj_same

        return check_obj_same(self, a, b)

    def test_fail(self):
        import pyyjson

        class A:
            pass

        # TypeError
        test_cases = {pyyjson.JSONEncodeError: [A(), 1 + 1j]}

        for err, case in test_cases.items():
            with self.subTest(msg=f"encoding_fail_test(case={case})"):
                with self.assertRaises(err):
                    pyyjson.dumps(case)

        # TODO uncomment this later
        # # MemoryError
        # test_cases = [
        #     [1] * 2147483648
        # ]

        # for case in test_cases:
        #     with self.subTest(msg=f'encoding_fail_test(case={case})'):
        #         with self.assertRaises(MemoryError):
        #             pyyjson.dumps(case)

    # def test_default(self):
    #     import pyyjson
    #     import json

    #     class A:
    #         def __init__(self, a):
    #             self.a = a

    #     def d4(obj):
    #         if isinstance(obj, A):
    #             return obj.a
    #         raise TypeError("Type not serializable")

    #     test_cases = [
    #         A("xsad"),
    #         [1, 2, A("2uiujdsa"), "cw"],
    #         {"a": 1, "b": A("d33qws")},
    #     ]

    #     for case in test_cases:
    #         with self.subTest(msg=f'encoding_default_test(case={case})'):
    #             result_json = json.dumps(case, default=d4, separators=(",", ":"))
    #             result_pyyjson = pyyjson.dumps(case, default=d4)
    #             self._check_obj_same(result_json, result_pyyjson)

    # def test_allow_nan(self):
    #     import pyyjson
    #     import json
    #     import math

    #     test_cases = [
    #         math.nan,
    #         math.inf,
    #         -math.inf,
    #         [1, 2, math.nan, math.inf, -math.inf],
    #         {"a": math.nan, "b": math.inf, "c": -math.inf},
    #     ]

    #     for case in test_cases:
    #         with self.subTest(msg=f'encoding_allow_nan_test(case={case})'):
    #             self.assertRaises(ValueError, pyyjson.dumps, case, allow_nan=False)
    #             self.assertRaises(ValueError, json.dumps, case, allow_nan=False, separators=(",", ":"))

    # def test_seperators(self):
    #     import pyyjson
    #     import json

    #     test_cases = [
    #         {"a": 1, "b": 2},
    #         [2, 5, 7],
    #         "a"
    #     ]

    #     for case in test_cases:
    #         with self.subTest(msg=f'encoding_separators_test(case={case})'):
    #             result_json = json.dumps(case, separators=(",-,-,", ":_:_:"))
    #             result_pyyjson = pyyjson.dumps(case, separators=(",-,-,", ":_:_:"))
    #             self._check_obj_same(result_json, result_pyyjson)

    # def test_skipkeys(self):
    #     import pyyjson
    #     import json

    #     class A:
    #         pass

    #     test_cases = [
    #         {"a": 1, 2: 3},
    #         {"a": "b", A(): 1},
    #         {A(): 1, "a": "b"},
    #         {"e": 2, A(): 1, "a": "b"},
    #     ]

    #     for case in test_cases:
    #         result_json = json.dumps(case, indent=None, separators=(",", ":"), ensure_ascii=False, skipkeys=True)
    #         result_loadback_json = json.loads(result_json)

    #         with self.subTest(msg=f'encoding_skipkeys_test(case={case})'):
    #             result_pyyjson = pyyjson.dumps(case, skipkeys=True)
    #             result_loadback_pyyjson = json.loads(result_pyyjson)
    #             self._check_obj_same(result_loadback_json, result_loadback_pyyjson)

    def test_encode(self):
        import collections
        import json
        import math

        from test_utils import get_benchfiles_fullpath

        import pyyjson

        test_cases = [
            [1, 2, 3, 4],  # simple list
            [1, 2.3, "a", None, True, False, [], {}],  # list
            ("a", 1, 2.3, 2.3, None, True, False, [], {}),  # tuple
            tuple(range(100)),  # large tuple
            # "a",  # simple string
            # 1,  # simple int
            # 2.3,  # simple float
            # math.inf,
            # math.nan,
            # math.pi,
            # None,
            # True,
            # False,
            [],
            {},
            # dict({a: b for a in range(10) for b in range(10)}),  # set
            {"啊啊啊": "ß"},
            # 321321432.231543245,  # large float
            # -321321432.231543245,
            # -1,
            # -2.3,
            # -math.inf,
            # -math.nan,
            # collections.defaultdict(x=1),
        ]

        for file in get_benchfiles_fullpath():
            with open(file, "r", encoding="utf-8") as f:
                test_cases.append(json.load(f))

        for case in test_cases:
            result_json = json.dumps(
                case, indent=None, separators=(",", ":"), ensure_ascii=False
            )
            result_loadback_json = json.loads(result_json)

            with self.subTest(msg=f"encoding_test(case={case})"):
                result_pyyjson = pyyjson.dumps(case)
                result_loadback_pyyjson = json.loads(result_pyyjson)
                self._check_obj_same(result_loadback_json, result_loadback_pyyjson)


if __name__ == "__main__":
    sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    sys.path.append("build")
    unittest.main()
