# SPDX-License-Identifier: (Apache-2.0 OR MIT)

import pytest

import pyyjson


class TestDict:
    def test_dict(self):
        """
        dict
        """
        obj = {"key": "value"}
        ref = '{"key":"value"}'
        assert pyyjson.dumps(obj) == ref
        assert pyyjson.dumps_to_bytes(obj) == ref.encode("utf-8")
        assert pyyjson.loads(ref) == obj

    def test_dict_duplicate_loads(self):
        assert pyyjson.loads(b'{"1":true,"1":false}') == {"1": False}

    def test_dict_empty(self):
        obj = [{"key": [{}] * 4096}] * 4096  # type:ignore
        assert pyyjson.loads(pyyjson.dumps(obj)) == obj
        assert pyyjson.loads(pyyjson.dumps_to_bytes(obj)) == obj

    def test_dict_large_dict(self):
        """
        dict with >512 keys
        """
        obj = {"key_%s" % idx: [{}, {"a": [{}, {}, {}]}, {}] for idx in range(513)}  # type: ignore
        assert len(obj) == 513
        assert pyyjson.loads(pyyjson.dumps(obj)) == obj
        assert pyyjson.loads(pyyjson.dumps_to_bytes(obj)) == obj

    def test_dict_large_4096(self):
        """
        dict with >4096 keys
        """
        obj = {"key_%s" % idx: "value_%s" % idx for idx in range(4097)}
        assert len(obj) == 4097
        assert pyyjson.loads(pyyjson.dumps(obj)) == obj
        assert pyyjson.loads(pyyjson.dumps_to_bytes(obj)) == obj

    def test_dict_large_65536(self):
        """
        dict with >65536 keys
        """
        obj = {"key_%s" % idx: "value_%s" % idx for idx in range(65537)}
        assert len(obj) == 65537
        assert pyyjson.loads(pyyjson.dumps(obj)) == obj
        assert pyyjson.loads(pyyjson.dumps_to_bytes(obj)) == obj

    def test_dict_large_keys(self):
        """
        dict with keys too large to cache
        """
        obj = {
            "keeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeey": "value"
        }
        ref = '{"keeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeey":"value"}'
        assert pyyjson.dumps(obj) == ref
        assert pyyjson.dumps_to_bytes(obj) == ref.encode("utf-8")
        assert pyyjson.loads(ref) == obj

    def test_dict_unicode(self):
        """
        dict unicode keys
        """
        obj = {"🐈": "value"}
        ref = b'{"\xf0\x9f\x90\x88":"value"}'.decode("utf-8")
        assert pyyjson.dumps(obj) == ref
        assert pyyjson.dumps_to_bytes(obj) == ref.encode("utf-8")
        assert pyyjson.loads(ref) == obj
        assert pyyjson.loads(ref)["🐈"] == "value"

    def test_dict_invalid_key_dumps(self):
        """
        dict invalid key dumps()
        """
        with pytest.raises(pyyjson.JSONEncodeError):
            pyyjson.dumps({1: "value"})
        with pytest.raises(pyyjson.JSONEncodeError):
            pyyjson.dumps({b"key": "value"})
        with pytest.raises(pyyjson.JSONEncodeError):
            pyyjson.dumps_to_bytes({1: "value"})
        with pytest.raises(pyyjson.JSONEncodeError):
            pyyjson.dumps_to_bytes({b"key": "value"})

    def test_dict_invalid_key_loads(self):
        """
        dict invalid key loads()
        """
        with pytest.raises(pyyjson.JSONDecodeError):
            pyyjson.loads('{1:"value"}')
        with pytest.raises(pyyjson.JSONDecodeError):
            pyyjson.loads('{{"a":true}:true}')

    def test_dict_similar_keys(self):
        """
        loads() similar keys

        This was a regression in 3.4.2 caused by using
        the implementation in wy instead of wyhash.
        """
        assert pyyjson.loads(
            '{"cf_status_firefox67": "---", "cf_status_firefox57": "verified"}'
        ) == {"cf_status_firefox57": "verified", "cf_status_firefox67": "---"}

    def test_dict_pop_replace_first(self):
        "Test pop and replace a first key in a dict with other keys."
        data = {"id": "any", "other": "any"}
        data.pop("id")
        assert pyyjson.dumps(data) == '{"other":"any"}'
        assert pyyjson.dumps_to_bytes(data) == b'{"other":"any"}'
        data["id"] = "new"
        assert pyyjson.dumps(data) == '{"other":"any","id":"new"}'
        assert pyyjson.dumps_to_bytes(data) == b'{"other":"any","id":"new"}'

    def test_dict_pop_replace_last(self):
        "Test pop and replace a last key in a dict with other keys."
        data = {"other": "any", "id": "any"}
        data.pop("id")
        assert pyyjson.dumps(data) == '{"other":"any"}'
        assert pyyjson.dumps_to_bytes(data) == b'{"other":"any"}'
        data["id"] = "new"
        assert pyyjson.dumps(data) == '{"other":"any","id":"new"}'
        assert pyyjson.dumps_to_bytes(data) == b'{"other":"any","id":"new"}'

    def test_dict_pop(self):
        "Test pop and replace a key in a dict with no other keys."
        data = {"id": "any"}
        data.pop("id")
        assert pyyjson.dumps(data) == "{}"
        assert pyyjson.dumps_to_bytes(data) == b"{}"
        data["id"] = "new"
        assert pyyjson.dumps(data) == '{"id":"new"}'
        assert pyyjson.dumps_to_bytes(data) == b'{"id":"new"}'

    def test_in_place(self):
        "Mutate dict in-place"
        data = {"id": "any", "static": "msg"}
        data["id"] = "new"
        assert pyyjson.dumps(data) == '{"id":"new","static":"msg"}'
        assert pyyjson.dumps_to_bytes(data) == b'{"id":"new","static":"msg"}'

    def test_dict_0xff(self):
        "dk_size <= 0xff"
        data = {str(idx): idx for idx in range(0, 0xFF)}
        data.pop("112")
        data["112"] = 1
        data["113"] = 2
        assert pyyjson.loads(pyyjson.dumps(data)) == data
        assert pyyjson.loads(pyyjson.dumps_to_bytes(data)) == data

    def test_dict_0xff_repeated(self):
        "dk_size <= 0xff repeated"
        for _ in range(0, 100):
            data = {str(idx): idx for idx in range(0, 0xFF)}
            data.pop("112")
            data["112"] = 1
            data["113"] = 2
            assert pyyjson.loads(pyyjson.dumps(data)) == data
            assert pyyjson.loads(pyyjson.dumps_to_bytes(data)) == data

    def test_dict_0xffff(self):
        "dk_size <= 0xffff"
        data = {str(idx): idx for idx in range(0, 0xFFFF)}
        data.pop("112")
        data["112"] = 1
        data["113"] = 2
        assert pyyjson.loads(pyyjson.dumps(data)) == data
        assert pyyjson.loads(pyyjson.dumps_to_bytes(data)) == data

    def test_dict_0xffff_repeated(self):
        "dk_size <= 0xffff repeated"
        for _ in range(0, 100):
            data = {str(idx): idx for idx in range(0, 0xFFFF)}
            data.pop("112")
            data["112"] = 1
            data["113"] = 2
            assert pyyjson.loads(pyyjson.dumps(data)) == data
            assert pyyjson.loads(pyyjson.dumps_to_bytes(data)) == data

    def test_dict_dict(self):
        class C:
            def __init__(self):
                self.a = 0
                self.b = 1

        assert pyyjson.dumps(C().__dict__) == '{"a":0,"b":1}'
        assert pyyjson.dumps_to_bytes(C().__dict__) == b'{"a":0,"b":1}'
