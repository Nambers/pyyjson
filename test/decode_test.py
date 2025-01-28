import os
import sys
import unittest


class TestDecode(unittest.TestCase):
    def _check_obj_same(self, a, b):
        from test_utils import check_obj_same

        return check_obj_same(self, a, b)

    def test_fail(self):
        import pyyjson

        test_cases = {
            ValueError: [
                "0xf",
                '{"a":}',
                "[1,2,3",
                '"111"]',
                '{"a": 1 2}'
            ],
            # OverflowError: ["1e500"]
        }
        for err, cases in test_cases.items():
            for case in cases:
                with self.subTest(msg=f"decoding_fail_test(case={case})"):
                    with self.assertRaises(err):
                        pyyjson.loads(case)

    # def test_object_hook(self):
    #     import pyyjson
    #     import json

    #     class A:
    #         def __init__(self, a):
    #             self.a = a

    #         def __eq__(self, other):
    #             return isinstance(other, A) and self.a == other.a

    #     def d4(obj):
    #         if "a" in obj:
    #             return A(obj["a"])
    #         return obj

    #     test_cases = [
    #         '{"a": 1}',
    #         '[{"a": 1}]',
    #         '{"a": {"a": 1}}',
    #         '[{"a": {"a": 1}}, {"b": 2}]',
    #     ]

    #     for case in test_cases:
    #         with self.subTest(msg=f'decoding_object_hook_test(case={case})'):
    #             result_json = json.loads(case, object_hook=d4)
    #             result_pyyjson = pyyjson.loads(case, object_hook=d4)
    #             self._check_obj_same(result_json, result_pyyjson)

    def test_decode(self):
        import collections
        import json
        import math

        import pyyjson

        test_cases_origin = [
            True,
            False,
            None,
            1,
            -1,
            2.3,
            -2.3,
            321321432.231543245,
            -321321432.231543245,
            "abc",
            math.inf,
            -math.inf,
            math.nan,
            math.pi,
            [],
            {},
            tuple(),
            [1, 2, 3, 4],
            [155, {}, 2.3, "a", None, True, False, [], {}, 11],
            ("a", 1, 2.3, 2.3, None, True, False, [], {}),
            {"啊啊啊": "ß", "ü": ["\uff02", "\u00f8"]},
            {"啊啊啊": "ß", "AnswerText": "This information is not held centrally.\r\n"},
            dict({a: b for a in range(10) for b in range(10)}),
            [[[[[[[[[[[[[[]]]]]]], [[[[[[[[[[[[[[[[[[[[]]]]]]]]]]]]]]]]]]]]]]]]]]],
            collections.OrderedDict(x=1),
            {"啊啊啊": "ß","AnswerText":"The A83 Trunk Road Route Study published in February 2013 considered a series of options with the objective to reduce the impact of the effects of landslides at the Rest and Be Thankful. This transport appraisal was undertaken in accordance with Scottish Transport Appraisal Guidance and Stage 1 scheme assessment in accordance with the Design Manual for Roads and Bridges.\r\nSix options were appraised and this is fully discussed in the A83 Trunk Road Route Study, Part A which can be found on the Transport Scotland website at the following link: http://www.transportscotland.gov.uk/system/files/uploaded_content/documents/projects/A83/a83-rest-and-be-thankful-project-a83-trunk-road-route-study-report-part-a-final.pdf\r\nThe “Brown Option” considered a 1km debris flow shelter. Following the initial sift against the stated criteria, the options for a flow-over canopy was sifted out for further consideration as other options were assessed to perform better against the appraisal criteria. Also similar benefits could be achieved with other options in the study at lower cost and with a lower potential environmental impact.\r\nThe Scottish Government remains committed to working with local communities and businesses to ensure that Argyll remains open for business. This has seen an investment of over £48 million in maintaining the A83 since 2007. This includes £9 million towards reducing the risk of impacts from landslides at the Rest and Be Thankful and the establishment of the local diversion route. The A83 taskforce will reconvene on 25 January 2016, when the continued work of the group and next steps will be discussed."}
        ]

        from test_utils import get_benchfiles_fullpath

        bench_files = get_benchfiles_fullpath()

        test_cases = [
            json.dumps(case, ensure_ascii=False) for case in test_cases_origin
        ] + [
            '{"a":"a", "a":"ab"}',  # repeated key
            '{"a":1, "b":[1.234]}',
            '["a", 1,null, "ab", 1.234, NaN, Infinity]',
        ]
        for bench_file in bench_files:
            with open(bench_file, "r", encoding="utf-8") as f:
                test_cases.append(f.read())

        for case in test_cases:
            # print(case)
            with self.subTest(msg=f"decoding_test(case={case})"):
                re_json = json.loads(case)
                re_pyyjson = pyyjson.loads(case)
                self._check_obj_same(re_pyyjson, re_json)


if __name__ == "__main__":
    sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    sys.path.append("build")
    unittest.main()
