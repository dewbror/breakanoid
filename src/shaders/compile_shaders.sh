#!/bin/bash

glslangValidator --target-env vulkan1.3 -V shader.frag
glslangValidator --target-env vulkan1.3 -V shader.vert
glslangValidator --target-env vulkan1.3 -V gradient.comp
