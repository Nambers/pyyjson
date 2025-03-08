# # SPDX-License-Identifier: (Apache-2.0 OR MIT)

# import collections
# import json

# import pytest

# import pyyjson


# class SubStr(str):
#     pass


# class SubInt(int):
#     pass


# class SubDict(dict):
#     pass


# class SubList(list):
#     pass


# class SubFloat(float):
#     pass


# class SubTuple(tuple):
#     pass


# class TestSubclass:
#     def test_subclass_str(self):
#         assert pyyjson.dumps(SubStr("zxc")) == b'"zxc"'

#     def test_subclass_str_invalid(self):
#         with pytest.raises(pyyjson.JSONEncodeError):
#             pyyjson.dumps(SubStr("\ud800"))

#     def test_subclass_int(self):
#         assert pyyjson.dumps(SubInt(1)) == b"1"

#     def test_subclass_int_64(self):
#         for val in (9223372036854775807, -9223372036854775807):
#             assert pyyjson.dumps(SubInt(val)) == str(val).encode("utf-8")

#     def test_subclass_int_53(self):
#         for val in (9007199254740992, -9007199254740992):
#             with pytest.raises(pyyjson.JSONEncodeError):
#                 pyyjson.dumps(SubInt(val), option=pyyjson.OPT_STRICT_INTEGER)

#     def test_subclass_dict(self):
#         assert pyyjson.dumps(SubDict({"a": "b"})) == b'{"a":"b"}'

#     def test_subclass_list(self):
#         assert pyyjson.dumps(SubList(["a", "b"])) == b'["a","b"]'
#         ref = [True] * 512
#         assert pyyjson.loads(pyyjson.dumps(SubList(ref))) == ref

#     def test_subclass_float(self):
#         with pytest.raises(pyyjson.JSONEncodeError):
#             pyyjson.dumps(SubFloat(1.1))
#         assert json.dumps(SubFloat(1.1)) == "1.1"

#     def test_subclass_tuple(self):
#         with pytest.raises(pyyjson.JSONEncodeError):
#             pyyjson.dumps(SubTuple((1, 2)))
#         assert json.dumps(SubTuple((1, 2))) == "[1, 2]"

#     def test_namedtuple(self):
#         Point = collections.namedtuple("Point", ["x", "y"])
#         with pytest.raises(pyyjson.JSONEncodeError):
#             pyyjson.dumps(Point(1, 2))

#     def test_subclass_circular_dict(self):
#         obj = SubDict({})
#         obj["obj"] = obj
#         with pytest.raises(pyyjson.JSONEncodeError):
#             pyyjson.dumps(obj)

#     def test_subclass_circular_list(self):
#         obj = SubList([])
#         obj.append(obj)
#         with pytest.raises(pyyjson.JSONEncodeError):
#             pyyjson.dumps(obj)

#     def test_subclass_circular_nested(self):
#         obj = SubDict({})
#         obj["list"] = SubList([{"obj": obj}])
#         with pytest.raises(pyyjson.JSONEncodeError):
#             pyyjson.dumps(obj)


# class TestSubclassPassthrough:
#     def test_subclass_str(self):
#         with pytest.raises(pyyjson.JSONEncodeError):
#             pyyjson.dumps(SubStr("zxc"), option=pyyjson.OPT_PASSTHROUGH_SUBCLASS)

#     def test_subclass_int(self):
#         with pytest.raises(pyyjson.JSONEncodeError):
#             pyyjson.dumps(SubInt(1), option=pyyjson.OPT_PASSTHROUGH_SUBCLASS)

#     def test_subclass_dict(self):
#         with pytest.raises(pyyjson.JSONEncodeError):
#             pyyjson.dumps(SubDict({"a": "b"}), option=pyyjson.OPT_PASSTHROUGH_SUBCLASS)

#     def test_subclass_list(self):
#         with pytest.raises(pyyjson.JSONEncodeError):
#             pyyjson.dumps(SubList(["a", "b"]), option=pyyjson.OPT_PASSTHROUGH_SUBCLASS)
