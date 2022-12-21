# gltfviewer

## A Windows library for viewing glTF files with the Cycles renderer
gltfviewer is a Windows dynamic link library for loading glTF/glb files and rendering them using Cycles (a path tracing renderer).

This project is an early work in progress, with the main priority to improve conformance with glTF sample models at https://github.com/KhronosGroup/glTF-Sample-Models/tree/master/2.0

### Project layout
The 'gltfviewer.sln' file can be found in the root folder, which requires Microsoft Visual Studio 2022. Debug and release build configurations are provided (x64 only). The solution contains two projects:
#### gltfviewer
The library itself, which builds to the 'output' folder, and consists of the 'gltfviewer.h' file, a 'gltfviewer.lib' file (in the 'output/lib' folder) and the 'gltfviewer.dll' file & third-party 'tbb.dll' dependency (in the output/bin folder).
#### testapp
A Windows test application which demonstrates library usage.

#### Libraries
The 'libraries' folder contains the third-party libraries required for the gltfviewer project. 

The Microsoft glTFSDK library is provided in precompiled form ('libraries\GLTFSDK'). 

The Cycles library is provided in precompiled form ('libraries\cycles\build\lib') and also in source form ('libraries\cycles\build\Cycles.sln'). The third-party libraries required by Cycles itself are provided in precompiled form ('libraries\lib\win64_vc15').

### Licenses
#### Cycles
https://www.cycles-renderer.org

Cycles is licensed under the Apache License 2.0

#### glTFSDK
https://github.com/microsoft/glTF-SDK

Copyright (c) Microsoft Corporation. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

#### JSON for Modern C++
https://github.com/nlohmann/json

Copyright (c) 2013-2022 Niels Lohmann

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

#### gltfviewer
https://github.com/jfchapman/gltfviewer

Copyright (c) James Chapman

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
