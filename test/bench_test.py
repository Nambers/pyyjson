import os
import sys
import unittest

# download bench folder from https://github.com/0ph1uch1/pycjson/tree/main/bench

class TestBenchmark(unittest.TestCase):
    BENCHMARK_REPEAT_SETTING = {
        "apache.json": 10000,
        "tweet.json": 200000,
        "MotionsQuestionsAnswersQuestions2016.json": 100,
        "canada.json": 250,
        "truenull.json":50000,
        "github.json":50000,
    }
    DEFAULT_REPEAT = 1000
    def setUp(self):
        try:
            import orjson

            self._orjson = orjson
        except ImportError:
            print("orjson is not installed")
            self._orjson = None

    def time_benchmark(self, repeat_time, func, *args, **kwargs):
        import time

        # warm up
        for _ in range(10):
            func(*args, **kwargs)

        time_0 = time.time()
        for _ in range(repeat_time):
            func(*args, **kwargs)
        time_1 = time.time()
        return time_1 - time_0

    def _run_test_setup(self, calling_setups, data, base_name, test_name):
        for setup in calling_setups:
            name, func, kwargs = setup
            repeat_times = self.BENCHMARK_REPEAT_SETTING.get(base_name, self.DEFAULT_REPEAT)
            time_ = self.time_benchmark(repeat_times, func, data, **kwargs)
            print(f"test {test_name}, file: {base_name}, time_{name}\t: {time_}")

    def test_benchmark_encode(self):
        import json

        from test_utils import get_benchfiles_fullpath

        import pyyjson

        bench_files = get_benchfiles_fullpath()

        std_calling_kwargs = {"ensure_ascii": False}
        std_json_setup = ("std", json.dumps, std_calling_kwargs)
        pyyjson_setup = ("pyyjson", pyyjson.dumps, {})

        calling_setups = [std_json_setup, pyyjson_setup]
        if self._orjson is not None:
            orjson = self._orjson
            def ordump(x):
                return orjson.dumps(x, option=orjson.OPT_INDENT_2).decode()
            orjson_setup = ("orjson", ordump, {})
            calling_setups.append(orjson_setup)

        for filename in bench_files:
            base_name = os.path.basename(filename)
            with open(filename, "r", encoding="utf-8") as f:
                data = json.load(f)
            self._run_test_setup(calling_setups, data, base_name, "encode")

    def test_benchmark_decode(self):
        import json

        from test_utils import get_benchfiles_fullpath

        import pyyjson

        bench_files = get_benchfiles_fullpath()

        std_json_setup = ("std", json.loads, {})
        pyyjson_setup = ("pyyjson", pyyjson.loads, {})
        calling_setups = [std_json_setup, pyyjson_setup]
        if self._orjson is not None:
            orjson_setup = ("orjson", self._orjson.loads, {})
            calling_setups.append(orjson_setup)

        for filename in bench_files:
            base_name = os.path.basename(filename)
            with open(filename, "r", encoding="utf-8") as f:
                data = f.read()
            self._run_test_setup(calling_setups, data, base_name, "decode")


if __name__ == "__main__":
    sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    sys.path.append("build")
    unittest.main()
