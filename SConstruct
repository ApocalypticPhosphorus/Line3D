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
    "project/addons/line3d/bin/{}/{}{}".format(
        env["platform"],
        libname,
        env["SHLIBSUFFIX"]
    ),
    source=sources,
)

Default(library)