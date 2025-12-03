import glob
import os
from pathlib import Path
from SCons.Script import DefaultEnvironment

env = DefaultEnvironment()

if os.uname().sysname == "Darwin":
  # Try to locate libclang_rt.profile_osx.a from Xcode/CLT
  candidates = glob.glob("/Library/Developer/CommandLineTools/usr/lib/clang/*/lib/darwin/libclang_rt.profile_osx.a")
  candidates += glob.glob("/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/lib/clang/*/lib/darwin/libclang_rt.profile_osx.a")
  candidates += glob.glob("/opt/homebrew/opt/llvm/lib/clang/*/lib/darwin/libclang_rt.profile_osx.a")

  if candidates:
    libpath = Path(sorted(candidates)[-1])
    env.Append(LINKFLAGS=[f"-L{libpath.parent}", "-lclang_rt.profile_osx"])
