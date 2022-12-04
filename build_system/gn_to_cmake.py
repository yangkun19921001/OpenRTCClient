#!/usr/bin/env python
#
# Copyright 2016 Google Inc.
#
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


"""
Usage: gn_to_cmake.py <json_file_name>
gn gen out/config --ide=json --json-ide-script=../../gn/gn_to_cmake.py
or
gn gen out/config --ide=json
python gn/gn_to_cmake.py out/config/project.json
The first is recommended, as it will auto-update.
"""


import itertools
import functools
import json
from operator import truediv
import posixpath
import os
import string
import sys

script_path = os.path.dirname(os.path.realpath(__file__))
script_path = script_path.replace('\\','/')

def IsWindows():
  if sys.platform.startswith(('cygwin', 'win')):
    return True
  return False

if IsWindows():
  ninjaCmd = 'ninja.exe'
else:
  ninjaCmd = 'ninja'

def CMakeTargetEscape(a):
  """Escapes the string 'a' for use as a CMake target name.
  CMP0037 in CMake 3.0 restricts target names to "^[A-Za-z0-9_.:+-]+$"
  The ':' is only allowed for imported targets.
  """
  def Escape(c):
    if c in string.ascii_letters or c in string.digits or c in '_.+-':
      return c
    else:
      return '__'
  return ''.join(map(Escape, a))

def GetTargetName(input) :
  vals = input.split(':')
  val0 = vals[0]
  if val0.startswith('//'):
    val0 = val0[2:]
  val0 = val0.replace('-', '_')
  prefixs = val0.split('/')
  result = '_'.join(prefixs) + '_' + vals[1]
  #print(CMakeTargetEscape(input),result)
  return  vals[-1]

def GetTargetSources(input, build_dir,root_dir):
  res_sources = []
  res_sources.append(posixpath.join(build_dir, 'empty.cpp'))
  if not 'sources' in input:
    return res_sources
  input_sources = input["sources"]
  for source in input_sources:
    if source.startswith('//'):
      res_sources.append(posixpath.join(root_dir, source[2:]))
    elif source.startswith('/') and sys.platform.startswith(('cygwin', 'win')):
      res_sources.append(source[1:])
    else:
      res_sources.append(source)
  return res_sources

def GetTargetIncludes(input, build_dir, root_dir):
  res_includes = []
  if not 'include_dirs' in input:
    return res_includes
  input_includes = input["include_dirs"]
  for include_dir in input_includes:
    if include_dir.startswith('//'):
      res_includes.append(posixpath.join(root_dir, include_dir[2:]))
    else:
      res_includes.append(include_dir)
  return res_includes

class Project(object):
  def __init__(self, project_json):
    self.targets = project_json['targets']
    build_settings = project_json['build_settings']
    self.root_path = build_settings['root_path']
    self.build_path = self.GetAbsolutePath(build_settings['build_dir'])

  def GetAbsolutePath(self, path):
    if path.startswith('//'):
      return posixpath.join(self.root_path, path[2:])
    elif path.startswith('/') and sys.platform.startswith(('cygwin', 'win')):
      return path[1:]
    else:
      return path


class Target(object):
  def __init__(self, project : Project, key, value,postfix):
    self.key = key
    self.build_path = project.build_path
    self.root_path = project.root_path
    self.target_name = GetTargetName(key)
    self.target_name = self.target_name + postfix
    self.source_name = self.target_name+'_'+'sources'
    self.sources = GetTargetSources(value, self.build_path, self.root_path)
    self.includes = GetTargetIncludes(value, self.build_path, self.root_path)
    self.defines = []
    self.output_name = None
    if 'output_name' in value:
      self.output_name = value["output_name"]
    if 'defines' in value:
      self.defines = value['defines']
    self.type = value['type']
    self.source_outputs = ""
    if 'source_outputs' in value:
      #tmp_dir = value['source_outputs'].values()
      for tmp_dir in value['source_outputs'].values():
        dir = posixpath.dirname(tmp_dir[0])
        if dir.startswith('//'):
          dir = posixpath.join(self.build_path, dir[2:])
        self.source_outputs = dir
        break
    
    if self.type == 'source_set':
      vals = self.key.split(':')
      val0 = vals[0]
      if val0.startswith('//'):
        val0 = val0[2:]
      self.output_dir = posixpath.join(self.build_path,'obj',val0)
      #print(self.target_name,self.output_dir)
    elif self.type == 'shared_library' or self.type == 'static_library':
      self.output_dir = value['outputs'][0]
      if self.output_dir.startswith('/') and sys.platform.startswith(('cygwin', 'win')):
        self.output_dir = posixpath.dirname(self.output_dir[1:])

  def Writable(self):
    if self.type == 'shared_library' or self.type == 'static_library' or self.type == 'source_set':
      return True
    return False
  def WriteTarget(self, output):
      if not self.Writable():
        return
      # set sources
      output.write('set({} \n'.format(self.source_name))
      for source in self.sources:
        output.write('  {}\n'.format(source))
      output.write(')\n\n')

      # add target
      target_name = self.target_name
      if self.type == 'shared_library':
        target_name = target_name + "_static"
      output.write('add_library({} {}\n'.format(target_name, 'STATIC'))
      output.write('  ${'+self.source_name+'}\n')
      output.write(')\n\n')

      # add defined
      output.write('target_compile_definitions({} PRIVATE\n'.format(target_name))
      for define in self.defines:
        output.write('  {}\n'.format(define))
      output.write(')\n\n')

      # add include
      output.write('target_include_directories({} PRIVATE\n'.format(target_name))
      for include in self.includes:
        output.write('  {}\n'.format(include))
      output.write(')\n\n')

      # add ninja command
      output.write('add_custom_command(TARGET {} PRE_BUILD\n'.format(target_name))
      output.write('  WORKING_DIRECTORY {}\n'.format(self.build_path))
      output.write('  COMMAND {} {}\n'.format(ninjaCmd,self.target_name))
      output.write(')\n\n')

      if self.type == 'shared_library':
        output.write('add_library({} {}\n'.format(self.target_name, 'SHARED'))
        empty_file = posixpath.join(self.build_path, 'empty.cpp')
        output.write('  '+empty_file+'\n')
        output.write(')\n\n')

        # add post command
        output.write('add_custom_command(TARGET {} POST_BUILD\n'.format(self.target_name))
        output.write('  WORKING_DIRECTORY {}\n'.format(self.build_path))
        output.write('  COMMAND ${{CMAKE_COMMAND}} -E copy {}lib.unstripped/lib{}.so $<TARGET_FILE:{}>\n'.format(
          self.build_path, self.output_name, self.target_name))
        output.write(')\n\n')

        # output_name
        if not self.output_name is None:
          output.write('set_target_properties({}\n'.format(self.target_name))
          output.write('  PROPERTIES\n')
          output.write('  OUTPUT_NAME "{}"\n'.format(self.output_name))
          output.write(')\n\n')

      # add  obj path
      # output.write('set_target_properties({} PROPERTIES\n'.format(self.target_name))
      # output.write('  ARCHIVE_OUTPUT_DIRECTORY_DEBUG {}\n'.format(self.output_dir))
      # output.write('  LIBRARY_OUTPUT_DIRECTORY_DEBUG {}\n'.format(self.output_dir))
      # output.write('  RUNTIME_OUTPUT_DIRECTORY_DEBUG {}\n'.format(self.output_dir))
      # output.write('  ARCHIVE_OUTPUT_DIRECTORY {}\n'.format(self.output_dir))
      # output.write('  LIBRARY_OUTPUT_DIRECTORY {}\n'.format(self.output_dir))
      # output.write('  RUNTIME_OUTPUT_DIRECTORY {}\n'.format(self.output_dir))
      # output.write(')\n\n')

      



def WriteProject(project):
  out = open(posixpath.join(project.build_path, 'CMakeLists.txt'), 'w+')
  out.write('# Generated by gn_to_cmake.py.\n')
  out.write('cmake_minimum_required(VERSION 3.5)\n')
  out.write('project(all_projects LANGUAGES CXX)\n\n')

  out.write('enable_language(C)\n')
  out.write('enable_language(CXX)\n')
  out.write('enable_language(ASM)\n')
  out.write('set(CMAKE_C_STANDARD 99)\n')
  out.write('set(CMAKE_CXX_STANDARD 17)\n\n')

  out.write('file(WRITE "')
  out.write(posixpath.join(project.build_path, "empty.cpp"))
  out.write('")\n')
  targets = {}
  dup_count = 1
  for target_name in project.targets.keys():
     target_n = GetTargetName(target_name)
     if not target_n in targets.keys():
      target = Target(project,target_name,project.targets[target_name],"")
     else:
      #print(target_n)
      target = Target(project,target_name,project.targets[target_name], str(dup_count))
      dup_count = dup_count + 1
     if target.Writable():
        out.write('\n')
        targets[target_n] = target
        #print(target.target_name)
        target.WriteTarget(out)
  
  out.write('add_library(all_projects STATIC\n')
  out.write('  {}\n'.format(posixpath.join(project.build_path, "empty.cpp")))
  out.write(')\n\n')

  # show project path in android studio
  out.write('target_include_directories(all_projects PRIVATE\n')
  out.write('  {}\n'.format(posixpath.join(script_path, "../gaming_sdk")))
  out.write('  {}\n'.format(posixpath.join(script_path, "../android_sdk")))
  out.write(')\n\n')

def main():
  if len(sys.argv) != 2:
    print('Usage: ' + sys.argv[0] + ' <json_file_name>')
    exit(1)

  json_path = sys.argv[1]
  project = None
  with open(json_path, 'r') as json_file:
    project = json.loads(json_file.read())

  WriteProject(Project(project))


if __name__ == "__main__":
  main()