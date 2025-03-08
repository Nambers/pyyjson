# # SPDX-License-Identifier: (Apache-2.0 OR MIT)

# import pyyjson

# from util import read_fixture_obj


# class TestDictSortKeys:
#     # citm_catalog is already sorted
#     def test_twitter_sorted(self):
#         """
#         twitter.json sorted
#         """
#         obj = read_fixture_obj("twitter.json.xz")
#         assert list(obj.keys()) != sorted(list(obj.keys()))
#         serialized = pyyjson.dumps(obj, option=pyyjson.OPT_SORT_KEYS)
#         val = pyyjson.loads(serialized)
#         assert list(val.keys()) == sorted(list(val.keys()))

#     def test_canada_sorted(self):
#         """
#         canada.json sorted
#         """
#         obj = read_fixture_obj("canada.json.xz")
#         assert list(obj.keys()) != sorted(list(obj.keys()))
#         serialized = pyyjson.dumps(obj, option=pyyjson.OPT_SORT_KEYS)
#         val = pyyjson.loads(serialized)
#         assert list(val.keys()) == sorted(list(val.keys()))

#     def test_github_sorted(self):
#         """
#         github.json sorted
#         """
#         obj = read_fixture_obj("github.json.xz")
#         for each in obj:
#             assert list(each.keys()) != sorted(list(each.keys()))
#         serialized = pyyjson.dumps(obj, option=pyyjson.OPT_SORT_KEYS)
#         val = pyyjson.loads(serialized)
#         for each in val:
#             assert list(each.keys()) == sorted(list(each.keys()))

#     def test_utf8_sorted(self):
#         """
#         UTF-8 sorted
#         """
#         obj = {"a": 1, "ä": 2, "A": 3}
#         assert list(obj.keys()) != sorted(list(obj.keys()))
#         serialized = pyyjson.dumps(obj, option=pyyjson.OPT_SORT_KEYS)
#         val = pyyjson.loads(serialized)
#         assert list(val.keys()) == sorted(list(val.keys()))
