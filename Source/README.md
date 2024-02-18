# Modules Documentation

## Module Folder Structure
- Source
  - **ModuleName**
    - **CompileGenerated**: Compile folder for the autogenerated code.
    - **PublicGenerated**: Public auto generated headers. 
    - **PrivateGenerated**: Autogenerated Code, macros and reflection system will be generated here.
      - DllDef.h: Linkage attributes, every module has a macro generated used for export and import symbols. 
      The linkage macro name is **\<MODULENAME\>_API**.
      But to make it easies to use, you can simply do **#include "WCore/WCore.h"**, which will manage the DllDef.h include.
    - **Include**: Header files to be included from the library, the module interface.
    - **Source**: Implementations of the module, you should add each source file into the CMakeLists.txt file.
    - **CMakeLists.txt**: All modules needs a CMakeLists file with the compile instructions.
    - **Shaders**: 
    - **Assets**: 