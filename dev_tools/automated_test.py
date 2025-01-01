import json
import os
import subprocess
import sys
import tempfile

CUR_FILE = os.path.abspath(__file__)
CUR_DIR = os.path.dirname(CUR_FILE)


class MockArgs(dict):
    def __setattr__(self, name, value):
        self[name] = value

    def __getattr__(self, name):
        return self.get(name)


def run_all_versions_test(args) -> int:
    with open("./dev_tools/supported_versions.json", "rb") as f:
        vers: list[int] = json.loads(f.read())
    exec_command_str = f"""
import importlib.util
spec = importlib.util.spec_from_file_location("autotest", "{CUR_FILE}")
mod = importlib.util.module_from_spec(spec)
spec.loader.exec_module(mod)
args = mod.MockArgs()
args.build_type = "{args.build_type}"
args.ignore = {args.ignore or []}
args.build_only = {bool(args.build_only)}
args.asan = {bool(args.asan)}
args.pyver = $TO_INSERT
mod.run_test(args)
"""
    print(exec_command_str)

    tempfiles = []
    for ver in vers:
        exec_command = exec_command_str.replace("$TO_INSERT", str(ver))
        file = tempfile.mktemp()
        with open(file, "w", encoding="utf-8") as f:
            f.write(exec_command)
        tempfiles.append(file)

    processes = []
    for file in tempfiles:
        process = subprocess.Popen([sys.executable, file])
        processes.append(process)

    has_error = False
    for i, process in enumerate(processes):
        process.wait()
        if process.returncode != 0:
            has_error = True
            print(f"Error when testing pyver: {vers[i]}")

    if not has_error:
        print("All tests passed.")

    # clean up
    for file in tempfiles:
        os.remove(file)

    return 1 if has_error else 0


def run_test(args):
    if args.ignore and args.build_only:
        print("Ignore test names only works when not building only.")
        return 1
    # if args.asan and args.build_only:
    #     print("ASAN check only works when not building only.")
    #     return 1
    if args.asan:
        print("NOTE: use asan check will suppress `ignore` and `build-type` options.")

    if args.all_ver:
        return run_all_versions_test(args)

    if sys.platform in ["linux", "darwin"]:
        # use nix
        test_entrance = os.path.join(CUR_DIR, "asan_check.sh" if args.asan else "autobuild.sh")
        envs = os.environ.copy()
        envs.update({
            "BUILD_PY_VER": str(args.pyver),
            "ISOLATE_BUILD": "1"
        })
        if not args.asan:
            build_type = args.build_type
            envs["TARGET_BUILD_TYPE"] = build_type
            if args.ignore:
                envs["IGNORES"] = " ".join(args.ignore)
        if args.build_only:
            envs["SKIP_TEST"] = "1"
        if "IN_NIX_SHELL" in os.environ:
            print("Already in nix shell")
            os.execvpe("bash", ["bash", test_entrance], envs)
        os.execvpe("nix", ["nix", "develop", "--command", test_entrance], envs)
    else:
        # Windows: not support --all-ver, use current python version
        import shutil
        from os.path import dirname
        from sysconfig import get_config_h_filename, get_config_var
        os.environ["Python3_EXECUTABLE"] = sys.executable
        os.environ["Python3_INCLUDE_DIR"] = dirname(get_config_h_filename())
        os.environ["Python3_LIBRARY"] = get_config_var("prefix") + os.path.sep + get_config_var("LDLIBRARY")
        if os.path.exists("build"):
            shutil.rmtree("build")
        os.makedirs("build")
        subprocess.run(["cmake", "-S", ".", "-B", "build", "-DCMAKE_BUILD_TYPE=" + args.build_type], check=True)
        subprocess.run(["cmake", "--build", "build", "--config", args.build_type], check=True)
        if args.build_only:
            return 0
        exe_base_name = os.path.basename(sys.executable)
        cmd = [exe_base_name, "test/all_test.py"]
        if args.ignore:
            cmd += ["--ignore"] + args.ignore
        target_file = f"build/{args.build_type}/pyyjson.dll"
        os.rename(target_file, "build/pyyjson.pyd")
        os.environ["PYTHONPATH"] = os.path.join(CUR_DIR, "..", "build")
        subprocess.run(cmd, check=True)
    return 0


def main():
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument("--build-type", help="CMake Build type", default="Debug")
    parser.add_argument("--pyver", help="Specify Python version, default to 12", default="12")
    parser.add_argument("--all-ver", help="Test with all versions", action="store_true")
    parser.add_argument("--ignore", help="Ignore test names", nargs="+", default=[])
    parser.add_argument("--build-only", help="Build without running tests", action="store_true")
    parser.add_argument("--asan", help="Run asan check", action="store_true")

    args = parser.parse_args()

    exit(run_test(args))


if __name__ == "__main__":
    main()
