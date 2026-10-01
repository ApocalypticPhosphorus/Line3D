#!/usr/bin/env python

import os

libname = "Line3D"
projectdir = "demo"

env = SConscript(
    "godot-cpp/SConstruct",
    {
        "api_version": "4.7"
    }
)

env.Append(CPPPATH=["src/"])

sources = Glob("src/*.cpp")

library = env.SharedLibrary(
    "bin/{}/{}{}".format(
        env["platform"],
        libname,
        env["SHLIBSUFFIX"]
    ),
    source=sources,
)

copy = env.Install(
    "{}/bin/{}/".format(projectdir, env["platform"]),
    library
)

Default(copy)