import os
from SCons.Script import DefaultEnvironment

env = DefaultEnvironment()

cc = os.getenv("CC", "gcc")
cxx = os.getenv("CXX", "g++")

env.Replace(CC=cc, CXX=cxx)
