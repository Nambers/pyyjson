# SPDX-License-Identifier: (Apache-2.0 OR MIT)


import json

import pytest

import pyyjson


class TestUltraJSON:
    def test_doubleLongIssue(self):
        sut = {"a": -4342969734183514}
        encoded = pyyjson.dumps(sut)
        decoded = pyyjson.loads(encoded)
        assert sut == decoded
        assert encoded == pyyjson.dumps_to_bytes(sut).decode("utf-8")
        encoded = pyyjson.dumps(sut)
        decoded = pyyjson.loads(encoded)
        assert sut == decoded
        assert encoded == pyyjson.dumps_to_bytes(sut).decode("utf-8")

    def test_doubleLongDecimalIssue(self):
        sut = {"a": -12345678901234.56789012}
        encoded = pyyjson.dumps(sut)
        decoded = pyyjson.loads(encoded)
        assert sut == decoded
        assert encoded == pyyjson.dumps_to_bytes(sut).decode("utf-8")
        encoded = pyyjson.dumps(sut)
        decoded = pyyjson.loads(encoded)
        assert sut == decoded
        assert encoded == pyyjson.dumps_to_bytes(sut).decode("utf-8")

    def test_encodeDecodeLongDecimal(self):
        sut = {"a": -528656961.4399388}
        encoded = pyyjson.dumps(sut)
        pyyjson.loads(encoded)
        assert encoded == pyyjson.dumps_to_bytes(sut).decode("utf-8")

    def test_decimalDecodeTest(self):
        sut = {"a": 4.56}
        encoded = pyyjson.dumps(sut)
        decoded = pyyjson.loads(encoded)
        pytest.approx(sut["a"], decoded["a"])
        assert encoded == pyyjson.dumps_to_bytes(sut).decode("utf-8")

    def test_encodeDictWithUnicodeKeys(self):
        val = {
            "key1": "value1",
            "key1": "value1",
            "key1": "value1",
            "key1": "value1",
            "key1": "value1",
            "key1": "value1",
        }
        assert pyyjson.dumps(val).encode("utf-8") == pyyjson.dumps_to_bytes(val)

        val = {
            "بن": "value1",
            "بن": "value1",
            "بن": "value1",
            "بن": "value1",
            "بن": "value1",
            "بن": "value1",
            "بن": "value1",
        }
        assert pyyjson.dumps(val).encode("utf-8") == pyyjson.dumps_to_bytes(val)

    def test_encodeArrayOfNestedArrays(self):
        val = [[[[]]]] * 20  # type: ignore
        output = pyyjson.dumps(val)
        assert val == pyyjson.loads(output)
        assert val == pyyjson.loads(output)
        assert output.encode("utf-8") == pyyjson.dumps_to_bytes(val)

    def test_encodeArrayOfDoubles(self):
        val = [31337.31337, 31337.31337, 31337.31337, 31337.31337] * 10
        output = pyyjson.dumps(val)
        assert val == pyyjson.loads(output)
        assert val == pyyjson.loads(output)
        assert output.encode("utf-8") == pyyjson.dumps_to_bytes(val)

    def test_encodeStringConversion2(self):
        val = "A string \\ / \b \f \n \r \t"
        output = pyyjson.dumps(val)
        assert val == pyyjson.loads(output)
        assert output == '"A string \\\\ / \\b \\f \\n \\r \\t"'
        assert val == pyyjson.loads(output)
        assert output.encode("utf-8") == pyyjson.dumps_to_bytes(val)

    def test_decodeUnicodeConversion(self):
        pass

    def test_encodeUnicodeConversion1(self):
        val = "Räksmörgås اسامة بن محمد بن عوض بن لادن"
        enc = pyyjson.dumps(val)
        dec = pyyjson.loads(enc)
        assert enc == pyyjson.dumps(val)
        assert dec == pyyjson.loads(enc)
        assert enc.encode("utf-8") == pyyjson.dumps_to_bytes(val)

    def test_encodeControlEscaping(self):
        val = "\x19"
        enc = pyyjson.dumps(val)
        dec = pyyjson.loads(enc)
        assert val == dec
        assert enc == pyyjson.dumps(val)
        assert enc.encode("utf-8") == pyyjson.dumps_to_bytes(val)

    def test_encodeUnicodeConversion2(self):
        val = "\xe6\x97\xa5\xd1\x88"
        enc = pyyjson.dumps(val)
        dec = pyyjson.loads(enc)
        assert enc == pyyjson.dumps(val)
        assert dec == pyyjson.loads(enc)
        assert enc.encode("utf-8") == pyyjson.dumps_to_bytes(val)

    def test_encodeUnicodeSurrogatePair(self):
        val = "\xf0\x90\x8d\x86"
        enc = pyyjson.dumps(val)
        dec = pyyjson.loads(enc)

        assert enc == pyyjson.dumps(val)
        assert dec == pyyjson.loads(enc)
        assert enc.encode("utf-8") == pyyjson.dumps_to_bytes(val)

    def test_encodeUnicode4BytesUTF8(self):
        val = "\xf0\x91\x80\xb0TRAILINGNORMAL"
        enc = pyyjson.dumps(val)
        dec = pyyjson.loads(enc)

        assert enc == pyyjson.dumps(val)
        assert dec == pyyjson.loads(enc)
        assert enc.encode("utf-8") == pyyjson.dumps_to_bytes(val)

    def test_encodeUnicode4BytesUTF8Highest(self):
        val = "\xf3\xbf\xbf\xbfTRAILINGNORMAL"
        enc = pyyjson.dumps(val)
        dec = pyyjson.loads(enc)

        assert enc == pyyjson.dumps(val)
        assert dec == pyyjson.loads(enc)
        assert enc.encode("utf-8") == pyyjson.dumps_to_bytes(val)

    def testEncodeUnicodeBMP(self):
        s = "\U0001f42e\U0001f42e\U0001f42d\U0001f42d"  # 🐮🐮🐭🐭
        pyyjson.dumps(s)
        json.dumps(s)

        assert json.loads(json.dumps(s)) == s
        assert pyyjson.loads(pyyjson.dumps(s)) == s
        assert pyyjson.loads(pyyjson.dumps_to_bytes(s)) == s

    def testEncodeSymbols(self):
        s = "\u273f\u2661\u273f"  # ✿♡✿
        encoded = pyyjson.dumps(s)
        encoded_json = json.dumps(s)

        decoded = pyyjson.loads(encoded)
        assert s == decoded

        encoded = pyyjson.dumps(s)
        assert encoded.encode("utf-8") == pyyjson.dumps_to_bytes(s)

        # json outputs an unicode object
        encoded_json = json.dumps(s, ensure_ascii=False)
        assert encoded == encoded_json
        decoded = pyyjson.loads(encoded)
        assert s == decoded

    def test_encodeArrayInArray(self):
        val = [[[[]]]]  # type: ignore
        output = pyyjson.dumps(val)

        assert val == pyyjson.loads(output)
        assert output == pyyjson.dumps(val)
        assert val == pyyjson.loads(output)
        assert output.encode("utf-8") == pyyjson.dumps_to_bytes(val)

    def test_encodeIntConversion(self):
        val = 31337
        output = pyyjson.dumps(val)
        assert val == pyyjson.loads(output)
        assert output == pyyjson.dumps(val)
        assert val == pyyjson.loads(output)
        assert output.encode("utf-8") == pyyjson.dumps_to_bytes(val)

    def test_encodeIntNegConversion(self):
        val = -31337
        output = pyyjson.dumps(val)
        assert val == pyyjson.loads(output)
        assert output == pyyjson.dumps(val)
        assert val == pyyjson.loads(output)
        assert output.encode("utf-8") == pyyjson.dumps_to_bytes(val)

    def test_encodeLongNegConversion(self):
        val = -9223372036854775808
        output = pyyjson.dumps(val)

        pyyjson.loads(output)
        pyyjson.loads(output)

        assert val == pyyjson.loads(output)
        assert output == pyyjson.dumps(val)
        assert val == pyyjson.loads(output)
        assert output.encode("utf-8") == pyyjson.dumps_to_bytes(val)

    def test_encodeListConversion(self):
        val = [1, 2, 3, 4]
        output = pyyjson.dumps(val)
        assert val == pyyjson.loads(output)
        assert val == pyyjson.loads(output)
        assert output.encode("utf-8") == pyyjson.dumps_to_bytes(val)

    def test_encodeDictConversion(self):
        val = {"k1": 1, "k2": 2, "k3": 3, "k4": 4}
        output = pyyjson.dumps(val)
        assert val == pyyjson.loads(output)
        assert val == pyyjson.loads(output)
        assert val == pyyjson.loads(output)
        assert output.encode("utf-8") == pyyjson.dumps_to_bytes(val)

    def test_encodeNoneConversion(self):
        val = None
        output = pyyjson.dumps(val)
        assert val == pyyjson.loads(output)
        assert output == pyyjson.dumps(val)
        assert val == pyyjson.loads(output)
        assert output.encode("utf-8") == pyyjson.dumps_to_bytes(val)

    def test_encodeTrueConversion(self):
        val = True
        output = pyyjson.dumps(val)
        assert val == pyyjson.loads(output)
        assert output == pyyjson.dumps(val)
        assert val == pyyjson.loads(output)
        assert output.encode("utf-8") == pyyjson.dumps_to_bytes(val)

    def test_encodeFalseConversion(self):
        val = False
        output = pyyjson.dumps(val)
        assert val == pyyjson.loads(output)
        assert output == pyyjson.dumps(val)
        assert val == pyyjson.loads(output)
        assert output.encode("utf-8") == pyyjson.dumps_to_bytes(val)

    def test_encodeToUTF8(self):
        val = b"\xe6\x97\xa5\xd1\x88".decode("utf-8")
        enc = pyyjson.dumps(val)
        dec = pyyjson.loads(enc)
        assert enc == pyyjson.dumps(val)
        assert dec == pyyjson.loads(enc)
        assert enc.encode("utf-8") == pyyjson.dumps_to_bytes(val)

    def test_decodeFromUnicode(self):
        val = '{"obj": 31337}'
        dec1 = pyyjson.loads(val)
        dec2 = pyyjson.loads(str(val))
        assert dec1 == dec2

    def test_decodeJibberish(self):
        val = "fdsa sda v9sa fdsa"
        pytest.raises(pyyjson.JSONDecodeError, pyyjson.loads, val)

    def test_decodeBrokenArrayStart(self):
        val = "["
        pytest.raises(pyyjson.JSONDecodeError, pyyjson.loads, val)

    def test_decodeBrokenObjectStart(self):
        val = "{"
        pytest.raises(pyyjson.JSONDecodeError, pyyjson.loads, val)

    def test_decodeBrokenArrayEnd(self):
        val = "]"
        pytest.raises(pyyjson.JSONDecodeError, pyyjson.loads, val)

    def test_decodeBrokenObjectEnd(self):
        val = "}"
        pytest.raises(pyyjson.JSONDecodeError, pyyjson.loads, val)

    def test_decodeObjectDepthTooBig(self):
        val = "{" * (1024 * 1024)
        pytest.raises(pyyjson.JSONDecodeError, pyyjson.loads, val)

    def test_decodeStringUnterminated(self):
        val = '"TESTING'
        pytest.raises(pyyjson.JSONDecodeError, pyyjson.loads, val)

    def test_decodeStringUntermEscapeSequence(self):
        val = '"TESTING\\"'
        pytest.raises(pyyjson.JSONDecodeError, pyyjson.loads, val)

    def test_decodeStringBadEscape(self):
        val = '"TESTING\\"'
        pytest.raises(pyyjson.JSONDecodeError, pyyjson.loads, val)

    def test_decodeTrueBroken(self):
        val = "tru"
        pytest.raises(pyyjson.JSONDecodeError, pyyjson.loads, val)

    def test_decodeFalseBroken(self):
        val = "fa"
        pytest.raises(pyyjson.JSONDecodeError, pyyjson.loads, val)

    def test_decodeNullBroken(self):
        val = "n"
        pytest.raises(pyyjson.JSONDecodeError, pyyjson.loads, val)

    def test_decodeBrokenDictKeyTypeLeakTest(self):
        val = '{{1337:""}}'
        for _ in range(1000):
            pytest.raises(pyyjson.JSONDecodeError, pyyjson.loads, val)

    def test_decodeBrokenDictLeakTest(self):
        val = '{{"key":"}'
        for _ in range(1000):
            pytest.raises(pyyjson.JSONDecodeError, pyyjson.loads, val)

    def test_decodeBrokenListLeakTest(self):
        val = "[[[true"
        for _ in range(1000):
            pytest.raises(pyyjson.JSONDecodeError, pyyjson.loads, val)

    def test_decodeDictWithNoKey(self):
        val = "{{{{31337}}}}"
        pytest.raises(pyyjson.JSONDecodeError, pyyjson.loads, val)

    def test_decodeDictWithNoColonOrValue(self):
        val = '{{{{"key"}}}}'
        pytest.raises(pyyjson.JSONDecodeError, pyyjson.loads, val)

    def test_decodeDictWithNoValue(self):
        val = '{{{{"key":}}}}'
        pytest.raises(pyyjson.JSONDecodeError, pyyjson.loads, val)

    def test_decodeNumericIntPos(self):
        val = "31337"
        assert 31337 == pyyjson.loads(val)

    def test_decodeNumericIntNeg(self):
        assert -31337 == pyyjson.loads("-31337")

    def test_encodeNullCharacter(self):
        val = "31337 \x00 1337"
        output = pyyjson.dumps(val)
        assert val == pyyjson.loads(output)
        assert output == pyyjson.dumps(val)
        assert val == pyyjson.loads(output)
        assert output.encode("utf-8") == pyyjson.dumps_to_bytes(val)

        val = "\x00"
        output = pyyjson.dumps(val)
        assert val == pyyjson.loads(output)
        assert output == pyyjson.dumps(val)
        assert val == pyyjson.loads(output)
        assert output.encode("utf-8") == pyyjson.dumps_to_bytes(val)

        assert '"  \\u0000\\r\\n "' == pyyjson.dumps("  \u0000\r\n ")
        assert b'"  \\u0000\\r\\n "' == pyyjson.dumps_to_bytes("  \u0000\r\n ")

    def test_decodeNullCharacter(self):
        val = '"31337 \\u0000 31337"'
        assert pyyjson.loads(val) == json.loads(val)

    def test_decodeEscape(self):
        base = "\u00e5".encode()
        quote = b'"'
        val = quote + base + quote
        assert json.loads(val) == pyyjson.loads(val)

    def test_decodeBigEscape(self):
        for _ in range(10):
            base = "\u00e5".encode()
            quote = b'"'
            val = quote + (base * 1024 * 1024 * 2) + quote
            assert json.loads(val) == pyyjson.loads(val)
