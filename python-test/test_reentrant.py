import pyyjson


class C:
    c: "C"

    def __del__(self):
        pyyjson.loads('"' + "a" * 10000 + '"')


def test_reentrant():
    c = C()
    c.c = c
    del c

    pyyjson.loads("[" + "[]," * 1000 + "[]]")
