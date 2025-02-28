#!/usr/bin/env python
# -*- coding: UTF-8 -*-
import os
import sys
import build_args_handler
import json
import utils
from configuration import Configuration
from utils import run_or_die
from utils import add_feature, get_platform_name, use_clang
from prepare_build import prepare_gen_android, prepare_build_android, prepare_gen_linux, prepare_gen_common


def prepare_static_common_gn_args(config):
  static_common_gn_args = [
    "is_component_build = false",
    # "rtc_build_examples = false\n",
    "rtc_build_tools = false\n",
    "rtc_include_tests = false\n",
    "treat_warnings_as_errors = false\n",
    "use_rtti = false"
  ]
  for arg in static_common_gn_args:
    add_feature(arg, config)

  if config.target_os == "linux" or config.target_os == "android" or config.is_windows:
    add_feature("use_dummy_lastchange = true\n", config)


def prepare_dynamic_common_gn_args(config):
  # os and cpu and is_debug
  add_feature('is_clang = {}\n'.format(use_clang(config)), config)
  if use_clang(config) == "false":
    add_feature('use_lld = {}\n'.format("false"), config)
  add_feature('target_cpu = "{}"\n'.format(config.target_cpu), config)
  add_feature('is_debug = {}\n'.format(config.is_debug), config)
  if config.is_debug == "false":
    if config.target_os == "ios" or config.target_os == "mac":
      add_feature("enable_dsyms = true\n", config)
      add_feature("enable_stripping = true\n", config)
    elif config.target_os == "win":
      add_feature("enable_iterator_debugging = false\n", config)
  else:
      if config.target_os == "linux":
        add_feature("is_asan = true\n", config)
      if config.target_os == "win":
        add_feature("enable_iterator_debugging = true\n", config)

  add_feature('symbol_level = {}\n'.format(2), config)
  # profile

  clang_base_path = os.path.join(os.path.dirname(os.path.realpath(__file__)), "llvm-build")

  if config.is_windows:
    clang_base_path = os.path.join(clang_base_path, "win")
    clang_base_path = os.path.join(clang_base_path, "Release+Asserts")
  elif config.is_mac:
    clang_base_path = os.path.join(clang_base_path, "mac")
    clang_base_path = os.path.join(clang_base_path, "mac")
    clang_base_path = os.path.join(clang_base_path, "Release+Asserts")
  elif config.is_linux:
    clang_base_path = os.path.join(clang_base_path, "linux")
    clang_base_path = os.path.join(clang_base_path, "linux")
    clang_base_path = os.path.join(clang_base_path, "Release+Asserts")

  add_feature('clang_base_path = "{}"\n'.format(clang_base_path), config)

  if config.target_os == "android":
    add_feature('android_sdk_build_tools_version = "{}"\n'.format(config.android_build_tools_ver), config)
    if config.is_mac:
      add_feature('android_sdk_tools_bundle_aapt2_dir = "{}"\n'.format(config.android_sdk_tools_bundle_aapt2_dir), config)


def prepare_predefined_gn_args(config):
  script_path = os.path.dirname(os.path.realpath(__file__))

  # predefined
  android_ndk = os.environ.get("ANDROID_NDK", "").replace("\\", "/")
  android_sdk = os.environ.get("ANDROID_SDK", "").replace("\\", "/")
  with open(os.path.join(script_path,
                         "./build_options_set/predefined_options_set.json"), "r") as f:
    predefined_gn_args = json.load(f)
  f.close()

  if not config.target_os in predefined_gn_args.keys():
    print("No predefined build options set for os {}".format(config.arget_os))
  else:
    args = predefined_gn_args.get(config.target_os, [])
    for arg in args:
      arg = arg.format(
        ANDROID_NDK=android_ndk,
        ANDROID_SDK=android_sdk,
        PLATFORM=get_platform_name(config))
      add_feature(arg, config)


# Custom build options set
def prepare_custom_gn_args(config):
  script_path = os.path.dirname(os.path.realpath(__file__))
  options_set_file = os.path.join(script_path, "build_options_set", "{}.conf".format(config.build_options_set))
  if not os.path.exists(options_set_file):
    print("Can not load profile {0} ({1})".format(config.build_options_set, options_set_file))
    sys.exit(-1)
  else:
    with open(options_set_file, "r") as of:
      feature = of.readline()
      while feature:
        add_feature(feature, config, True)
        feature = of.readline()
      of.close()


def prepare_gn_args(config):
  prepare_static_common_gn_args(config)
  prepare_dynamic_common_gn_args(config)
  prepare_predefined_gn_args(config)
  prepare_custom_gn_args(config)

  if "rtc_use_h264" not in config.gn_args_config or config.gn_args_config["rtc_use_h264"] == "false":
    if "ffmpeg_branding" in config.gn_args_config:
      del config.gn_args_config["ffmpeg_branding"]

  # write to args.gn
  file_name = os.path.join(os.getcwd(), config.build_dir, "args.gn")
  if os.path.exists(file_name):
    os.remove(file_name)

  with open(file_name, "w") as f:
    for k, v in config.gn_args_config.items():
      f.write('{} = {}\n'.format(k, v))


def parse_args(args, config):
  for v in build_args_handler.ARGS_HANDLER:
    getattr(build_args_handler, v)(argv=args, config=config)


def get_generator(config):
  if config.build_command != "gen":
    generator = ""

  generator = config.generator
  if generator == "default":
    generator = ""
    if config.is_windows:
      generator = "vs"
    if config.is_mac and not config.target_os == "android":
      generator = "xcode   --xcode-build-system=new"
    if config.target_os == "android":
      generator = "json --json-ide-script={}/build_system/gn_to_cmake.py".format(config.root_path)
  return generator


def generate_solution(config):
  prepare_gen_common(config)
  if config.target_os == "android":
    prepare_gen_android(config)
    prepare_build_android(config)
  if config.target_os == "linux":
    prepare_gen_linux(config)

  prepare_gn_args(config)

  generator = get_generator(config)
  ide_arg = "--ide={}".format(generator) if len(generator) != 0 else ""
  cmd = "{0} gen {1} --root={2} {3}".format(
    config.gn_bin_path, config.build_dir, config.webrtc_path, ide_arg
  )

  run_or_die(cmd)


def build_solution(config):
  cmd = "{0} -C {1} {2} {3} {4}".format(config.ninja_bin_path, config.build_dir,
                                        config.build_target, "-v" if config.verbos_build else "",
                                        " -j {}".format(
                                        config.limit_num_process) if config.limit_num_process else "")
  print("ninja exec: ", cmd)  # 添加这行来打印 cmd
  run_or_die(cmd)
  if not config.is_windows:
    print("\033[1;32m build success \033[0m")
  else:
    print("build success")


def clean_solution(config):
  cmd = "{0} clean {1} --root={2}".format(
    config.gn_bin_path, config.build_dir, config.webrtc_path
  )
  print("cmd",  cmd)
  run_or_die(cmd)


def find_absolute_target(config):
  cmd = "{0} ls {1} --root={2}".format(
    config.gn_bin_path, config.build_dir, config.webrtc_path
  )
  ret, output = utils.run_cmd(cmd, 2)
  found_target = None
  for target in output.readlines():
    target = target.decode()[:-1]
    target = target.strip()
    if config.build_target == target:
      found_target = target
      break

    paths = target.split(':')
    if len(paths) == 2 and paths[1] == config.build_target:
      found_target = target
      break
  output.close()
  return found_target


def print_target_description(config):
  target = find_absolute_target(config)
  cmd = "{0} desc {1} --root={2} {3}".format(
    config.gn_bin_path, config.build_dir, config.webrtc_path, target
  )
  run_or_die(cmd)


def print_target_deps_tree(config):
  target = find_absolute_target(config)
  cmd = "{0} desc {1} --root={2} {3} deps --tree".format(
    config.gn_bin_path, config.build_dir, config.webrtc_path, target
  )
  run_or_die(cmd)


def list_targets(config):
  cmd = "{0} ls {1} --root={2}".format(
    config.gn_bin_path, config.build_dir, config.webrtc_path
  )
  run_or_die(cmd)


def list_args(config):
  cmd = "{0} args --list {1} --root={2}".format(
    config.gn_bin_path, config.build_dir, config.webrtc_path
  )
  run_or_die(cmd)


COMMANDS = {
    "gen": lambda x: generate_solution(x),
    "build": lambda x: build_solution(x),
    "clean": lambda x: clean_solution(x),
    "info": lambda x: print_target_description(x),
    "deps_tree": lambda x: print_target_deps_tree(x),
    "list": lambda x: list_targets(x),
    "args": lambda x: list_args(x),
}


def exec_build(args):
  script_path = os.path.dirname(os.path.realpath(__file__))
  config = Configuration()
  config.init_path_config(script_path)
  config.init_command_config(script_path)

  parse_args(args, config)

  func = COMMANDS.get(config.build_command)
  func(config)
  return config


# for example: gen win x64 debug
# for example: gen win x64 debug iot
# for example: build:webrtc win x64 debug
def main(args):
  exec_build(args)


if __name__ == '__main__':
  try:
    sys.exit(main(sys.argv[1:]))
  except KeyboardInterrupt:
    sys.stderr.write('interrupted\n')
    sys.exit(1)
