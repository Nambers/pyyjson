import gc
import json
import os
import subprocess
from collections import defaultdict
from typing import Any, Callable

import orjson
import pyyjson

CUR_FILE = os.path.abspath(__file__)
CUR_DIR = os.path.dirname(CUR_FILE)


class ObjectDict(dict):
    def __setattr__(self, name, value):
        self[name] = value

    def __getattr__(self, name):
        return self.get(name)


class LibrarySetting(ObjectDict):
    function_catagory: str
    orjson_function: Callable
    pyyjson_function: Callable
    orjson_function_name: str
    pyyjson_function_name: str


def benchmark(repeat_time: int, func, *args):
    """
    Run repeat benchmark, disabling orjson utf-8 cache.
    returns time used (ns).
    """
    # warm up
    pyyjson.run_object_accumulate_benchmark(func, 100, args)
    return pyyjson.run_object_accumulate_benchmark(func, repeat_time, args)


def benchmark_unicode_arg(repeat_time: int, func, unicode: str, *args):
    """
    Run repeat benchmark, disabling orjson utf-8 cache.
    returns time used (ns).
    """
    # warm up
    pyyjson.run_unicode_accumulate_benchmark(func, 100, unicode, args)
    return pyyjson.run_unicode_accumulate_benchmark(func, repeat_time, unicode, args)


def benchmark_use_dump_cache(repeat_time: int, func, raw_bytes: bytes, *args):
    """
    let orjson use utf-8 cache for the same input.
    returns time used (ns).
    """
    new_args = (json.loads(raw_bytes), *args)
    # warm up
    for _ in range(100):
        pyyjson.run_object_benchmark(func, new_args)
    #
    total = 0
    for _ in range(repeat_time):
        total += pyyjson.run_object_benchmark(func, new_args)
    return total


def benchmark_invalidate_dump_cache(repeat_time: int, func, raw_bytes: bytes, *args):
    """
    orjson will use utf-8 cache for the same input,
    so we need to invalidate it.
    returns time used (ns).
    """
    # warm up
    for _ in range(10):
        new_args = (json.loads(raw_bytes), *args)
        pyyjson.run_object_benchmark(func, new_args)
    #
    total = 0
    for _ in range(repeat_time):
        new_args = (json.loads(raw_bytes), *args)
        total += pyyjson.run_object_benchmark(func, new_args)
    return total


def get_benchmark_libraries():
    dumps_setting = LibrarySetting()
    dumps_setting.function_catagory = "dumps"
    dumps_setting.orjson_function = lambda x: orjson.dumps(x).decode("utf-8")
    dumps_setting.orjson_function = orjson.dumps
    dumps_setting.pyyjson_function = pyyjson.dumps
    dumps_setting.orjson_function_name = "orjson.dumps"
    dumps_setting.pyyjson_function_name = "pyyjson.dumps"

    loads_setting_str = LibrarySetting()
    loads_setting_str.function_catagory = "loads(str)"
    loads_setting_str.orjson_function = orjson.loads
    loads_setting_str.pyyjson_function = pyyjson.loads
    loads_setting_str.orjson_function_name = "orjson.loads"
    loads_setting_str.pyyjson_function_name = "pyyjson.loads"

    loads_setting_bytes = LibrarySetting()
    loads_setting_bytes.function_catagory = "loads(bytes)"
    loads_setting_bytes.orjson_function = orjson.loads
    loads_setting_bytes.pyyjson_function = pyyjson.loads
    loads_setting_bytes.orjson_function_name = "orjson.loads"
    loads_setting_bytes.pyyjson_function_name = "pyyjson.loads"

    return dumps_setting, loads_setting_str, loads_setting_bytes


def get_benchmark_files():
    return sorted(
        list(
            map(
                lambda x: os.path.join(CUR_DIR, x),
                filter(lambda x: x.endswith(".json"), os.listdir(CUR_DIR)),
            )
        )
    )


def run_file_benchmark(
    file: str, result: defaultdict[str, defaultdict[str, Any]], process_bytes: int
):
    # if not file.endswith("apache.json"):
    #     return
    dumps_setting, loads_setting_str, loads_setting_bytes = get_benchmark_libraries()
    with open(file, "rb") as f:
        raw_bytes = f.read()
    raw = raw_bytes.decode("utf-8")
    base_file_name = os.path.basename(file)
    curfile_obj = result[base_file_name]
    curfile_obj["byte_size"] = bytes_size = len(raw_bytes)
    kind, str_size, is_ascii, _ = pyyjson.inspect_pyunicode(raw)
    curfile_obj["pyunicode_size"] = str_size
    curfile_obj["pyunicode_kind"] = kind
    curfile_obj["pyunicode_is_ascii"] = is_ascii
    repeat_times = (process_bytes + bytes_size - 1) // bytes_size
    real_process_bytes = repeat_times * bytes_size
    process_str_size = repeat_times * str_size
    # dumps
    cur_obj = curfile_obj[dumps_setting.function_catagory]
    gc.collect()
    cur_obj[dumps_setting.orjson_function_name] = orjson_time = (
        benchmark_invalidate_dump_cache(
            repeat_times, dumps_setting.orjson_function, raw_bytes
        )
    )
    gc.collect()
    cur_obj[dumps_setting.pyyjson_function_name] = pyyjson_time = (
        benchmark_invalidate_dump_cache(
            repeat_times, dumps_setting.pyyjson_function, raw_bytes
        )
    )
    cur_obj["ratio"] = pyyjson_time / orjson_time
    cur_obj["pyyjson_bytes_per_sec"] = pyyjson.dumps(
        real_process_bytes / (pyyjson_time / 1000000000)
    )
    # loads (str)
    cur_obj = curfile_obj[loads_setting_str.function_catagory]
    gc.collect()
    cur_obj[loads_setting_str.orjson_function_name] = orjson_time = (
        benchmark_unicode_arg(repeat_times, loads_setting_str.orjson_function, raw)
    )
    gc.collect()
    cur_obj[loads_setting_str.pyyjson_function_name] = pyyjson_time = (
        benchmark_unicode_arg(repeat_times, loads_setting_str.pyyjson_function, raw)
    )
    cur_obj["ratio"] = pyyjson_time / orjson_time
    cur_obj["pyyjson_bytes_per_sec"] = pyyjson.dumps(
        process_str_size / (pyyjson_time / 1000000000)
    )
    # loads (bytes)
    cur_obj = curfile_obj[loads_setting_bytes.function_catagory]
    gc.collect()
    cur_obj[loads_setting_bytes.orjson_function_name] = orjson_time = benchmark(
        repeat_times, loads_setting_bytes.orjson_function, raw_bytes
    )
    gc.collect()
    cur_obj[loads_setting_bytes.pyyjson_function_name] = pyyjson_time = benchmark(
        repeat_times, loads_setting_bytes.pyyjson_function, raw_bytes
    )
    cur_obj["ratio"] = pyyjson_time / orjson_time
    cur_obj["pyyjson_bytes_per_sec"] = pyyjson.dumps(
        real_process_bytes / (pyyjson_time / 1000000000)
    )


def get_head_rev_name():
    try:
        return subprocess.check_output(
            ["git", "rev-parse", "--short", "HEAD"],
            stderr=subprocess.DEVNULL,
            encoding="utf-8",
        ).strip()
    except subprocess.CalledProcessError:
        return ""


def get_real_output_file_name(output: str):
    if output:
        file = output
    else:
        rev = get_head_rev_name()
        if not rev:
            file = "benchmark_result.json"
        else:
            file = f"benchmark_result_{rev}.json"
    return file


def run_benchmark(process_bytes: int, output: str, is_stdout: bool = False):
    file = get_real_output_file_name(output)
    if os.path.exists(file):
        os.remove(file)
    result: defaultdict[str, defaultdict[str, Any]] = defaultdict(
        lambda: defaultdict(dict)
    )

    for bench_file in get_benchmark_files():
        run_file_benchmark(bench_file, result, process_bytes)
    output_result = json.dumps(result, indent=4)
    with open(file, "w", encoding="utf-8") as f:
        f.write(output_result)
    if is_stdout:
        print(output_result)


def main():
    import argparse

    parser = argparse.ArgumentParser()

    parser.add_argument(
        "-o", "--output", help="Output file", required=False, default=None
    )
    parser.add_argument(
        "--process-bytes",
        help="Total process bytes per test, default 1e8",
        required=False,
        default=100000000,
        type=int,
    )
    parser.add_argument(
        "--stdout", help="Print to stdout", required=False, action="store_true"
    )
    args = parser.parse_args()

    run_benchmark(args.process_bytes, args.output, args.stdout)


if __name__ == "__main__":
    main()
