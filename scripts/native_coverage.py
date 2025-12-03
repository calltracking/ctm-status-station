Import("env")

# ensure gcov runtime is linked for native coverage build
env.Append(LINKFLAGS=["--coverage", "-lgcov"])
