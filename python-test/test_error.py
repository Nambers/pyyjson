# # SPDX-License-Identifier: (Apache-2.0 OR MIT)

# import json

# import pytest

# import pyyjson

# from util import read_fixture_str

# ASCII_TEST = b"""\
# {
#   "a": "qwe",
#   "b": "qweqwe",
#   "c": "qweq",
#   "d: "qwe"
# }
# """

# MULTILINE_EMOJI = """[
#     "😊",
#     "a"
# """


# class TestJsonDecodeError:
#     # TODO
#     def _get_error_infos(self, json_decode_error_exc_info):
#         return {
#             k: v
#             for k, v in json_decode_error_exc_info.value.__dict__.items()
#             if k in ("pos", "lineno", "colno")
#         }

#     def _test(self, data, expected_err_infos):
#         with pytest.raises(json.decoder.JSONDecodeError) as json_exc_info:
#             json.loads(data)

#         with pytest.raises(json.decoder.JSONDecodeError) as pyyjson_exc_info:
#             pyyjson.loads(data)

#         assert (
#             self._get_error_infos(json_exc_info)
#             == self._get_error_infos(pyyjson_exc_info)
#             == expected_err_infos
#         )

#     def test_empty(self):
#         with pytest.raises(pyyjson.JSONDecodeError) as json_exc_info:
#             pyyjson.loads("")
#         assert str(json_exc_info.value).startswith(
#             "input data is empty"
#         )

#     def test_ascii(self):
#         self._test(
#             ASCII_TEST,
#             {"pos": 55, "lineno": 5, "colno": 8},
#         )

#     def test_latin1(self):
#         self._test(
#             """["üýþÿ", "a" """,
#             {"pos": 13, "lineno": 1, "colno": 14},
#         )

#     def test_two_byte_str(self):
#         self._test(
#             """["東京", "a" """,
#             {"pos": 11, "lineno": 1, "colno": 12},
#         )

#     def test_two_byte_bytes(self):
#         self._test(
#             b'["\xe6\x9d\xb1\xe4\xba\xac", "a" ',
#             {"pos": 11, "lineno": 1, "colno": 12},
#         )

#     def test_four_byte(self):
#         self._test(
#             MULTILINE_EMOJI,
#             {"pos": 19, "lineno": 4, "colno": 1},
#         )

#     # def test_tab(self):
#     #     data = read_fixture_str("fail26.json", "jsonchecker")
#     #     with pytest.raises(json.decoder.JSONDecodeError) as json_exc_info:
#     #         json.loads(data)

#     #     assert self._get_error_infos(json_exc_info) == {
#     #         "pos": 5,
#     #         "lineno": 1,
#     #         "colno": 6,
#     #     }

#     #     with pytest.raises(json.decoder.JSONDecodeError) as json_exc_info:
#     #         pyyjson.loads(data)

#     #     assert self._get_error_infos(json_exc_info) == {
#     #         "pos": 6,
#     #         "lineno": 1,
#     #         "colno": 7,
#     #     }


# class Custom:
#     pass


# class CustomException(Exception):
#     pass


# def default_typeerror(obj):
#     raise TypeError


# def default_notimplementederror(obj):
#     raise NotImplementedError


# def default_systemerror(obj):
#     raise SystemError


# def default_importerror(obj):
#     import doesnotexist

#     assert doesnotexist


# CUSTOM_ERROR_MESSAGE = "zxc"


# def default_customerror(obj):
#     raise CustomException(CUSTOM_ERROR_MESSAGE)


# class TestJsonEncodeError:
#     def test_dumps_arg(self):
#         with pytest.raises(pyyjson.JSONEncodeError) as exc_info:
#             pyyjson.dumps()  # type: ignore
#         assert exc_info.type == pyyjson.JSONEncodeError
#         assert (
#             str(exc_info.value)
#             == "dumps() missing 1 required positional argument: 'obj'"
#         )
#         assert exc_info.value.__cause__ is None

#     def test_dumps_chain_none(self):
#         with pytest.raises(pyyjson.JSONEncodeError) as exc_info:
#             pyyjson.dumps(Custom())
#         assert exc_info.type == pyyjson.JSONEncodeError
#         assert str(exc_info.value) == "Type is not JSON serializable: Custom"
#         assert exc_info.value.__cause__ is None

#     def test_dumps_chain_u64(self):
#         with pytest.raises(pyyjson.JSONEncodeError) as exc_info:
#             pyyjson.dumps([18446744073709551615, Custom()])
#         assert exc_info.type == pyyjson.JSONEncodeError
#         assert exc_info.value.__cause__ is None

#     def test_dumps_chain_default_typeerror(self):
#         with pytest.raises(pyyjson.JSONEncodeError) as exc_info:
#             pyyjson.dumps(Custom(), default=default_typeerror)
#         assert exc_info.type == pyyjson.JSONEncodeError
#         assert isinstance(exc_info.value.__cause__, TypeError)

#     def test_dumps_chain_default_systemerror(self):
#         with pytest.raises(pyyjson.JSONEncodeError) as exc_info:
#             pyyjson.dumps(Custom(), default=default_systemerror)
#         assert exc_info.type == pyyjson.JSONEncodeError
#         assert isinstance(exc_info.value.__cause__, SystemError)

#     def test_dumps_chain_default_importerror(self):
#         with pytest.raises(pyyjson.JSONEncodeError) as exc_info:
#             pyyjson.dumps(Custom(), default=default_importerror)
#         assert exc_info.type == pyyjson.JSONEncodeError
#         assert isinstance(exc_info.value.__cause__, ImportError)

#     def test_dumps_chain_default_customerror(self):
#         with pytest.raises(pyyjson.JSONEncodeError) as exc_info:
#             pyyjson.dumps(Custom(), default=default_customerror)
#         assert exc_info.type == pyyjson.JSONEncodeError
#         assert isinstance(exc_info.value.__cause__, CustomException)
#         assert str(exc_info.value.__cause__) == CUSTOM_ERROR_MESSAGE

#     def test_dumps_normalize_exception(self):
#         with pytest.raises(pyyjson.JSONEncodeError) as exc_info:
#             pyyjson.dumps(10**60)
#         assert exc_info.type == pyyjson.JSONEncodeError
#         assert isinstance(exc_info.value.__cause__, OverflowError)
