import os
import sys
import json
import unittest

# def test_encode_option(files):
#     import pyyjson
#     import tempfile
#     for file in files:
#         with open(file, "r", encoding="utf-8") as f:
#             json_data = json.load(f)
#         with tempfile.NamedTemporaryFile(mode="w+", encoding='ascii') as f:
#             pyyjson.dump(json_data, f)


# def test_decode_option(files):
#     import pyyjson
#     for file in files:
#         with open(file, "r", encoding="ascii") as f:
#             pyyjson_data = pyyjson.load(f)


class TestJsonFileIO(unittest.TestCase):
    def _check_obj_same(self, a, b):
        from test_utils import check_obj_same

        return check_obj_same(self, a, b)

    def setUp(self):
        from test_utils import get_benchfiles_fullpath

        self.file_path = get_benchfiles_fullpath()
        self.datas = []
        for file in get_benchfiles_fullpath():
            with open(file, "r", encoding="utf-8") as f:
                self.datas.append(json.load(f))

    def test_json_load(self):
        import pyyjson

        class A:
            def __init__(self, a):
                self.a = a

            def __eq__(self, other):
                return isinstance(other, A) and self.a == other.a

        def d4(obj):
            if "a" in obj:
                return A(obj["a"])
            return obj

        for file in self.file_path:
            # test parse keyword
            # with open(file, "r+", encoding="utf-8") as f:
            #     pyyjson_data = pyyjson.load(object_hook=d4, fp=f)
            #     f.seek(0)
            #     json_data = json.load(f)
            # self._check_obj_same(pyyjson_data, json_data)

            with open(file, "r+", encoding="utf-8") as f:
                pyyjson_data = pyyjson.loads(f.read())
                f.seek(0)
                json_data = json.loads(f.read())
            self._check_obj_same(pyyjson_data, json_data)

    def test_json_dump(self):
        import pyyjson
        import tempfile

        for data in self.datas:
            # test parse keyword
            # with tempfile.NamedTemporaryFile(mode="w+", encoding='utf-8') as f:
            #     pyyjson.dump(skipkeys=False, fp=f, obj=data)
            #     f.seek(0)
            #     pyyjson_data = json.load(f)
            # with tempfile.NamedTemporaryFile(mode="w+", encoding='utf-8') as f:
            #     json.dump(data, f)
            #     f.seek(0)
            #     json_data = json.load(f)
            # self._check_obj_same(pyyjson_data, json_data)

            self._check_obj_same(json.loads(pyyjson.dumps(data)), data)

    # file contents may null
    # def test_wrong_encoding(self):
    # self.assertRaises(ValueError, test_encode_option, self.file_path)
    # self.assertRaises(ValueError, test_decode_option, self.file_path)


if __name__ == "__main__":
    sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    sys.path.append("build")
    unittest.main()
