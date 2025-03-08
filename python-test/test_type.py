# # SPDX-License-Identifier: (Apache-2.0 OR MIT)

import io
import sys

import pytest

try:
    import xxhash
except ImportError:
    xxhash = None

import pyyjson


class TestType:
    def test_fragment(self):
        """
        pyyjson.JSONDecodeError on fragments
        """
        for val in ("n", "{", "[", "t"):
            pytest.raises(pyyjson.JSONDecodeError, pyyjson.loads, val)

    def test_invalid(self):
        """
        pyyjson.JSONDecodeError on invalid
        """
        for val in ('{"age", 44}', "[31337,]", "[,31337]", "[]]", "[,]"):
            pytest.raises(pyyjson.JSONDecodeError, pyyjson.loads, val)

    def test_str(self):
        """
        str
        """
        for obj, ref in (("blah", '"blah"'), ("東京", b'"\xe6\x9d\xb1\xe4\xba\xac"'.decode("utf-8"))):
            assert pyyjson.dumps(obj) == ref
            assert pyyjson.loads(ref) == obj

    def test_str_latin1(self):
        """
        str latin1
        """
        assert pyyjson.loads(pyyjson.dumps("üýþÿ")) == "üýþÿ"

    def test_str_long(self):
        """
        str long
        """
        for obj in ("aaaa" * 1024, "üýþÿ" * 1024, "好" * 1024, "�" * 1024):
            assert pyyjson.loads(pyyjson.dumps(obj)) == obj

    def test_str_2mib(self):
        ref = '🐈🐈🐈🐈🐈"üýa0s9999🐈🐈🐈🐈🐈9\0999\\9999' * 1 * 1
        assert pyyjson.loads(pyyjson.dumps(ref)) == ref

#     def test_str_very_long(self):
#         """
#         str long enough to trigger overflow in bytecount
#         """
#         for obj in ("aaaa" * 20000, "üýþÿ" * 20000, "好" * 20000, "�" * 20000):
#             assert pyyjson.loads(pyyjson.dumps(obj)) == obj

#     def test_str_replacement(self):
#         """
#         str roundtrip �
#         """
#         assert pyyjson.dumps("�") == b'"\xef\xbf\xbd"'
#         assert pyyjson.loads(b'"\xef\xbf\xbd"') == "�"

#     def test_str_trailing_4_byte(self):
#         ref = "うぞ〜😏🙌"
#         assert pyyjson.loads(pyyjson.dumps(ref)) == ref

#     def test_str_ascii_control(self):
#         """
#         worst case format_escaped_str_with_escapes() allocation
#         """
#         ref = "\x01\x1f" * 1024 * 16
#         assert pyyjson.loads(pyyjson.dumps(ref)) == ref
#         assert pyyjson.loads(pyyjson.dumps(ref, option=pyyjson.OPT_INDENT_2)) == ref

#     def test_str_escape_quote_0(self):
#         assert pyyjson.dumps('"aaaaaaabb') == b'"\\"aaaaaaabb"'

#     def test_str_escape_quote_1(self):
#         assert pyyjson.dumps('a"aaaaaabb') == b'"a\\"aaaaaabb"'

#     def test_str_escape_quote_2(self):
#         assert pyyjson.dumps('aa"aaaaabb') == b'"aa\\"aaaaabb"'

#     def test_str_escape_quote_3(self):
#         assert pyyjson.dumps('aaa"aaaabb') == b'"aaa\\"aaaabb"'

#     def test_str_escape_quote_4(self):
#         assert pyyjson.dumps('aaaa"aaabb') == b'"aaaa\\"aaabb"'

#     def test_str_escape_quote_5(self):
#         assert pyyjson.dumps('aaaaa"aabb') == b'"aaaaa\\"aabb"'

#     def test_str_escape_quote_6(self):
#         assert pyyjson.dumps('aaaaaa"abb') == b'"aaaaaa\\"abb"'

#     def test_str_escape_quote_7(self):
#         assert pyyjson.dumps('aaaaaaa"bb') == b'"aaaaaaa\\"bb"'

#     def test_str_escape_quote_8(self):
#         assert pyyjson.dumps('aaaaaaaab"') == b'"aaaaaaaab\\""'

#     def test_str_escape_quote_multi(self):
#         assert (
#             pyyjson.dumps('aa"aaaaabbbbbbbbbbbbbbbbbbbb"bb')
#             == b'"aa\\"aaaaabbbbbbbbbbbbbbbbbbbb\\"bb"'
#         )

#     def test_str_escape_backslash_0(self):
#         assert pyyjson.dumps("\\aaaaaaabb") == b'"\\\\aaaaaaabb"'

#     def test_str_escape_backslash_1(self):
#         assert pyyjson.dumps("a\\aaaaaabb") == b'"a\\\\aaaaaabb"'

#     def test_str_escape_backslash_2(self):
#         assert pyyjson.dumps("aa\\aaaaabb") == b'"aa\\\\aaaaabb"'

#     def test_str_escape_backslash_3(self):
#         assert pyyjson.dumps("aaa\\aaaabb") == b'"aaa\\\\aaaabb"'

#     def test_str_escape_backslash_4(self):
#         assert pyyjson.dumps("aaaa\\aaabb") == b'"aaaa\\\\aaabb"'

#     def test_str_escape_backslash_5(self):
#         assert pyyjson.dumps("aaaaa\\aabb") == b'"aaaaa\\\\aabb"'

#     def test_str_escape_backslash_6(self):
#         assert pyyjson.dumps("aaaaaa\\abb") == b'"aaaaaa\\\\abb"'

#     def test_str_escape_backslash_7(self):
#         assert pyyjson.dumps("aaaaaaa\\bb") == b'"aaaaaaa\\\\bb"'

#     def test_str_escape_backslash_8(self):
#         assert pyyjson.dumps("aaaaaaaab\\") == b'"aaaaaaaab\\\\"'

#     def test_str_escape_backslash_multi(self):
#         assert (
#             pyyjson.dumps("aa\\aaaaabbbbbbbbbbbbbbbbbbbb\\bb")
#             == b'"aa\\\\aaaaabbbbbbbbbbbbbbbbbbbb\\\\bb"'
#         )

#     def test_str_escape_x32_0(self):
#         assert pyyjson.dumps("\taaaaaaabb") == b'"\\taaaaaaabb"'

#     def test_str_escape_x32_1(self):
#         assert pyyjson.dumps("a\taaaaaabb") == b'"a\\taaaaaabb"'

#     def test_str_escape_x32_2(self):
#         assert pyyjson.dumps("aa\taaaaabb") == b'"aa\\taaaaabb"'

#     def test_str_escape_x32_3(self):
#         assert pyyjson.dumps("aaa\taaaabb") == b'"aaa\\taaaabb"'

#     def test_str_escape_x32_4(self):
#         assert pyyjson.dumps("aaaa\taaabb") == b'"aaaa\\taaabb"'

#     def test_str_escape_x32_5(self):
#         assert pyyjson.dumps("aaaaa\taabb") == b'"aaaaa\\taabb"'

#     def test_str_escape_x32_6(self):
#         assert pyyjson.dumps("aaaaaa\tabb") == b'"aaaaaa\\tabb"'

#     def test_str_escape_x32_7(self):
#         assert pyyjson.dumps("aaaaaaa\tbb") == b'"aaaaaaa\\tbb"'

#     def test_str_escape_x32_8(self):
#         assert pyyjson.dumps("aaaaaaaab\t") == b'"aaaaaaaab\\t"'

#     def test_str_escape_x32_multi(self):
#         assert (
#             pyyjson.dumps("aa\taaaaabbbbbbbbbbbbbbbbbbbb\tbb")
#             == b'"aa\\taaaaabbbbbbbbbbbbbbbbbbbb\\tbb"'
#         )

#     def test_str_emoji(self):
#         ref = "®️"
#         assert pyyjson.loads(pyyjson.dumps(ref)) == ref

#     def test_str_emoji_escape(self):
#         ref = '/"®️/"'
#         assert pyyjson.loads(pyyjson.dumps(ref)) == ref

#     def test_very_long_list(self):
#         pyyjson.dumps([[]] * 1024 * 16)

#     def test_very_long_list_pretty(self):
#         pyyjson.dumps([[]] * 1024 * 16, option=pyyjson.OPT_INDENT_2)

#     def test_very_long_dict(self):
#         pyyjson.dumps([{}] * 1024 * 16)

#     def test_very_long_dict_pretty(self):
#         pyyjson.dumps([{}] * 1024 * 16, option=pyyjson.OPT_INDENT_2)

#     def test_very_long_str_empty(self):
#         pyyjson.dumps([""] * 1024 * 16)

#     def test_very_long_str_empty_pretty(self):
#         pyyjson.dumps([""] * 1024 * 16, option=pyyjson.OPT_INDENT_2)

#     def test_very_long_str_not_empty(self):
#         pyyjson.dumps(["a"] * 1024 * 16)

#     def test_very_long_str_not_empty_pretty(self):
#         pyyjson.dumps(["a"] * 1024 * 16, option=pyyjson.OPT_INDENT_2)

#     def test_very_long_bool(self):
#         pyyjson.dumps([True] * 1024 * 16)

#     def test_very_long_bool_pretty(self):
#         pyyjson.dumps([True] * 1024 * 16, option=pyyjson.OPT_INDENT_2)

#     def test_very_long_int(self):
#         pyyjson.dumps([(2**64) - 1] * 1024 * 16)

#     def test_very_long_int_pretty(self):
#         pyyjson.dumps([(2**64) - 1] * 1024 * 16, option=pyyjson.OPT_INDENT_2)

#     def test_very_long_float(self):
#         pyyjson.dumps([sys.float_info.max] * 1024 * 16)

#     def test_very_long_float_pretty(self):
#         pyyjson.dumps([sys.float_info.max] * 1024 * 16, option=pyyjson.OPT_INDENT_2)

#     def test_str_surrogates_loads(self):
#         """
#         str unicode surrogates loads()
#         """
#         pytest.raises(pyyjson.JSONDecodeError, pyyjson.loads, '"\ud800"')
#         pytest.raises(pyyjson.JSONDecodeError, pyyjson.loads, '"\ud83d\ude80"')
#         pytest.raises(pyyjson.JSONDecodeError, pyyjson.loads, '"\udcff"')
#         pytest.raises(
#             pyyjson.JSONDecodeError, pyyjson.loads, b'"\xed\xa0\xbd\xed\xba\x80"'
#         )  # \ud83d\ude80

#     def test_str_surrogates_dumps(self):
#         """
#         str unicode surrogates dumps()
#         """
#         pytest.raises(pyyjson.JSONEncodeError, pyyjson.dumps, "\ud800")
#         pytest.raises(pyyjson.JSONEncodeError, pyyjson.dumps, "\ud83d\ude80")
#         pytest.raises(pyyjson.JSONEncodeError, pyyjson.dumps, "\udcff")
#         pytest.raises(pyyjson.JSONEncodeError, pyyjson.dumps, {"\ud83d\ude80": None})
#         pytest.raises(
#             pyyjson.JSONEncodeError, pyyjson.dumps, b"\xed\xa0\xbd\xed\xba\x80"
#         )  # \ud83d\ude80

#     @pytest.mark.skipif(
#         xxhash is None, reason="xxhash install broken on win, python3.9, Azure"
#     )
#     def test_str_ascii(self):
#         """
#         str is ASCII but not compact
#         """
#         digest = xxhash.xxh32_hexdigest("12345")
#         for _ in range(2):
#             assert pyyjson.dumps(digest) == b'"b30d56b4"'

#     def test_bytes_dumps(self):
#         """
#         bytes dumps not supported
#         """
#         with pytest.raises(pyyjson.JSONEncodeError):
#             pyyjson.dumps([b"a"])

#     def test_bytes_loads(self):
#         """
#         bytes loads
#         """
#         assert pyyjson.loads(b"[]") == []

#     def test_bytearray_loads(self):
#         """
#         bytearray loads
#         """
#         arr = bytearray()
#         arr.extend(b"[]")
#         assert pyyjson.loads(arr) == []

#     def test_memoryview_loads(self):
#         """
#         memoryview loads
#         """
#         arr = bytearray()
#         arr.extend(b"[]")
#         assert pyyjson.loads(memoryview(arr)) == []

#     def test_bytesio_loads(self):
#         """
#         memoryview loads
#         """
#         arr = io.BytesIO(b"[]")
#         assert pyyjson.loads(arr.getbuffer()) == []

#     def test_bool(self):
#         """
#         bool
#         """
#         for obj, ref in ((True, "true"), (False, "false")):
#             assert pyyjson.dumps(obj) == ref.encode("utf-8")
#             assert pyyjson.loads(ref) == obj

#     def test_bool_true_array(self):
#         """
#         bool true array
#         """
#         obj = [True] * 256
#         ref = ("[" + ("true," * 255) + "true]").encode("utf-8")
#         assert pyyjson.dumps(obj) == ref
#         assert pyyjson.loads(ref) == obj

#     def test_bool_false_array(self):
#         """
#         bool false array
#         """
#         obj = [False] * 256
#         ref = ("[" + ("false," * 255) + "false]").encode("utf-8")
#         assert pyyjson.dumps(obj) == ref
#         assert pyyjson.loads(ref) == obj

#     def test_none(self):
#         """
#         null
#         """
#         obj = None
#         ref = "null"
#         assert pyyjson.dumps(obj) == ref.encode("utf-8")
#         assert pyyjson.loads(ref) == obj

#     def test_int(self):
#         """
#         int compact and non-compact
#         """
#         obj = [-5000, -1000, -10, -5, -2, -1, 0, 1, 2, 5, 10, 1000, 50000]
#         ref = b"[-5000,-1000,-10,-5,-2,-1,0,1,2,5,10,1000,50000]"
#         assert pyyjson.dumps(obj) == ref
#         assert pyyjson.loads(ref) == obj

#     def test_null_array(self):
#         """
#         null array
#         """
#         obj = [None] * 256
#         ref = ("[" + ("null," * 255) + "null]").encode("utf-8")
#         assert pyyjson.dumps(obj) == ref
#         assert pyyjson.loads(ref) == obj

#     def test_nan_dumps(self):
#         """
#         NaN serializes to null
#         """
#         assert pyyjson.dumps(float("NaN")) == b"null"

#     def test_nan_loads(self):
#         """
#         NaN is not valid JSON
#         """
#         with pytest.raises(pyyjson.JSONDecodeError):
#             pyyjson.loads("[NaN]")
#         with pytest.raises(pyyjson.JSONDecodeError):
#             pyyjson.loads("[nan]")

#     def test_infinity_dumps(self):
#         """
#         Infinity serializes to null
#         """
#         assert pyyjson.dumps(float("Infinity")) == b"null"

#     def test_infinity_loads(self):
#         """
#         Infinity, -Infinity is not valid JSON
#         """
#         with pytest.raises(pyyjson.JSONDecodeError):
#             pyyjson.loads("[infinity]")
#         with pytest.raises(pyyjson.JSONDecodeError):
#             pyyjson.loads("[Infinity]")
#         with pytest.raises(pyyjson.JSONDecodeError):
#             pyyjson.loads("[-Infinity]")
#         with pytest.raises(pyyjson.JSONDecodeError):
#             pyyjson.loads("[-infinity]")

#     def test_int_53(self):
#         """
#         int 53-bit
#         """
#         for val in (9007199254740991, -9007199254740991):
#             assert pyyjson.loads(str(val)) == val
#             assert pyyjson.dumps(val, option=pyyjson.OPT_STRICT_INTEGER) == str(
#                 val
#             ).encode("utf-8")

#     def test_int_53_exc(self):
#         """
#         int 53-bit exception on 64-bit
#         """
#         for val in (9007199254740992, -9007199254740992):
#             with pytest.raises(pyyjson.JSONEncodeError):
#                 pyyjson.dumps(val, option=pyyjson.OPT_STRICT_INTEGER)

#     def test_int_53_exc_usize(self):
#         """
#         int 53-bit exception on 64-bit usize
#         """
#         for val in (9223372036854775808, 18446744073709551615):
#             with pytest.raises(pyyjson.JSONEncodeError):
#                 pyyjson.dumps(val, option=pyyjson.OPT_STRICT_INTEGER)

#     def test_int_64(self):
#         """
#         int 64-bit
#         """
#         for val in (9223372036854775807, -9223372036854775807):
#             assert pyyjson.loads(str(val)) == val
#             assert pyyjson.dumps(val) == str(val).encode("utf-8")

#     def test_uint_64(self):
#         """
#         uint 64-bit
#         """
#         for val in (0, 9223372036854775808, 18446744073709551615):
#             assert pyyjson.loads(str(val)) == val
#             # assert pyyjson.dumps(val) == str(val).encode("utf-8")

#     def test_int_128(self):
#         """
#         int 128-bit
#         """
#         for val in (18446744073709551616, -9223372036854775809):
#             pytest.raises(pyyjson.JSONEncodeError, pyyjson.dumps, val)

#     def test_float(self):
#         """
#         float
#         """
#         assert -1.1234567893 == pyyjson.loads("-1.1234567893")
#         assert -1.234567893 == pyyjson.loads("-1.234567893")
#         assert -1.34567893 == pyyjson.loads("-1.34567893")
#         assert -1.4567893 == pyyjson.loads("-1.4567893")
#         assert -1.567893 == pyyjson.loads("-1.567893")
#         assert -1.67893 == pyyjson.loads("-1.67893")
#         assert -1.7893 == pyyjson.loads("-1.7893")
#         assert -1.893 == pyyjson.loads("-1.893")
#         assert -1.3 == pyyjson.loads("-1.3")

#         assert 1.1234567893 == pyyjson.loads("1.1234567893")
#         assert 1.234567893 == pyyjson.loads("1.234567893")
#         assert 1.34567893 == pyyjson.loads("1.34567893")
#         assert 1.4567893 == pyyjson.loads("1.4567893")
#         assert 1.567893 == pyyjson.loads("1.567893")
#         assert 1.67893 == pyyjson.loads("1.67893")
#         assert 1.7893 == pyyjson.loads("1.7893")
#         assert 1.893 == pyyjson.loads("1.893")
#         assert 1.3 == pyyjson.loads("1.3")

#     def test_float_precision_loads(self):
#         """
#         float precision loads()
#         """
#         assert pyyjson.loads("31.245270191439438") == 31.245270191439438
#         assert pyyjson.loads("-31.245270191439438") == -31.245270191439438
#         assert pyyjson.loads("121.48791951161945") == 121.48791951161945
#         assert pyyjson.loads("-121.48791951161945") == -121.48791951161945
#         assert pyyjson.loads("100.78399658203125") == 100.78399658203125
#         assert pyyjson.loads("-100.78399658203125") == -100.78399658203125

#     def test_float_precision_dumps(self):
#         """
#         float precision dumps()
#         """
#         assert pyyjson.dumps(31.245270191439438) == b"31.245270191439438"
#         assert pyyjson.dumps(-31.245270191439438) == b"-31.245270191439438"
#         assert pyyjson.dumps(121.48791951161945) == b"121.48791951161945"
#         assert pyyjson.dumps(-121.48791951161945) == b"-121.48791951161945"
#         assert pyyjson.dumps(100.78399658203125) == b"100.78399658203125"
#         assert pyyjson.dumps(-100.78399658203125) == b"-100.78399658203125"

#     def test_float_edge(self):
#         """
#         float edge cases
#         """
#         assert pyyjson.dumps(0.8701) == b"0.8701"

#         assert pyyjson.loads("0.8701") == 0.8701
#         assert (
#             pyyjson.loads("0.0000000000000000000000000000000000000000000000000123e50")
#             == 1.23
#         )
#         assert pyyjson.loads("0.4e5") == 40000.0
#         assert pyyjson.loads("0.00e-00") == 0.0
#         assert pyyjson.loads("0.4e-001") == 0.04
#         assert pyyjson.loads("0.123456789e-12") == 1.23456789e-13
#         assert pyyjson.loads("1.234567890E+34") == 1.23456789e34
#         assert pyyjson.loads("23456789012E66") == 2.3456789012e76

#     def test_float_notation(self):
#         """
#         float notation
#         """
#         for val in ("1.337E40", "1.337e+40", "1337e40", "1.337E-4"):
#             obj = pyyjson.loads(val)
#             assert obj == float(val)
#             assert pyyjson.dumps(val) == ('"%s"' % val).encode("utf-8")

#     def test_list(self):
#         """
#         list
#         """
#         obj = ["a", "😊", True, {"b": 1.1}, 2]
#         ref = '["a","😊",true,{"b":1.1},2]'
#         assert pyyjson.dumps(obj) == ref.encode("utf-8")
#         assert pyyjson.loads(ref) == obj

#     def test_tuple(self):
#         """
#         tuple
#         """
#         obj = ("a", "😊", True, {"b": 1.1}, 2)
#         ref = '["a","😊",true,{"b":1.1},2]'
#         assert pyyjson.dumps(obj) == ref.encode("utf-8")
#         assert pyyjson.loads(ref) == list(obj)

#     def test_object(self):
#         """
#         object() dumps()
#         """
#         with pytest.raises(pyyjson.JSONEncodeError):
#             pyyjson.dumps(object())
