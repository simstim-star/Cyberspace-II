CC = cl.exe
LINK = link.exe

# --- Project Files ---
PROJECT_NAME = __cyberspaceII

# --- Dynamic Directory Logic ---
BUILD_ROOT_DEBUG = build_nmake\debug
BUILD_ROOT_PROFILE = build_nmake\profile

!IF "$(DEBUG)" == "1"
BUILD_ROOT = $(BUILD_ROOT_DEBUG)
CONF_CFLAGS = /Od /Zi /D_DEBUG /MTd
CONF_LDFLAGS = /DEBUG
!ELSEIF "$(PROFILE)" == "1"               
BUILD_ROOT = $(BUILD_ROOT_PROFILE)
CONF_CFLAGS = /O2 /Zi /DNDEBUG /MT
CONF_LDFLAGS = /DEBUG /PROFILE
!ELSE
BUILD_ROOT = build_nmake\release
CONF_CFLAGS = /O2 /DNDEBUG /MT
CONF_LDFLAGS = /INCREMENTAL:NO
!ENDIF

# Directory Setup
OBJ_DIR = $(BUILD_ROOT)\obj
OUT_DIR = $(BUILD_ROOT)\bin

# --- Compilation Flags ---
INCLUDES = /external:I"C:\xmathc" /external:I"deps" /I"src" /I"src\core"
DEFINES = /DUNICODE /D_UNICODE /D_XM_VECTORCALL_ /D_CRT_SECURE_NO_WARNINGS

# CHANGE: Switched /std:c11 to /std:c++20 for std::format and wstring_view
# Added /EHsc for C++ exception handling
CFLAGS = /nologo /W4 /std:c++20 /EHsc /external:W0 $(INCLUDES) $(DEFINES) $(CONF_CFLAGS) /FI"pch.h" /Fd"$(BUILD_ROOT)/"

PCH_HEADER_STR = pch.h
PCH_FILE = $(OBJ_DIR)\pch.pch
PCH_OBJ = $(OBJ_DIR)\pch.obj

# --- Linker Flags ---
LIBS = d3d12.lib dxguid.lib dxgi.lib D3DCompiler.lib user32.lib gdi32.lib shell32.lib ole32.lib
LDFLAGS = /nologo /SUBSYSTEM:WINDOWS /MACHINE:X64 $(CONF_LDFLAGS)

# --- Objects ---
ALL_OBJS = \
    $(OBJ_DIR)\core\camera.obj \
    $(OBJ_DIR)\core\timer.obj \
    $(OBJ_DIR)\win32\win_main.obj \
    $(OBJ_DIR)\win32\win_path.obj \
    $(OBJ_DIR)\win32\file_dialog.obj \
    $(OBJ_DIR)\renderer\renderer.obj \
    $(OBJ_DIR)\renderer\light.obj \
    $(OBJ_DIR)\renderer\texture.obj \
    $(OBJ_DIR)\renderer\shader.obj \
    $(OBJ_DIR)\renderer\billboard.obj \
    $(OBJ_DIR)\error\error.obj \
    $(OBJ_DIR)\core\engine.obj \
    $(OBJ_DIR)\core\grid.obj \
    $(OBJ_DIR)\core\log.obj \
    $(OBJ_DIR)\assets\gltf.obj \
    $(OBJ_DIR)\core\memory.obj

# --- Build Rules ---

all: setup $(PCH_OBJ) $(OUT_DIR)\$(PROJECT_NAME).exe copy_shaders copy_assets

setup:
    @if not exist build_nmake mkdir build_nmake
    @if not exist $(BUILD_ROOT) mkdir $(BUILD_ROOT)
    @if not exist $(OUT_DIR) mkdir $(OUT_DIR)
    @if not exist $(OBJ_DIR) mkdir $(OBJ_DIR)
    @if not exist $(OBJ_DIR)\core mkdir $(OBJ_DIR)\core
    @if not exist $(OBJ_DIR)\win32 mkdir $(OBJ_DIR)\win32
    @if not exist $(OBJ_DIR)\renderer mkdir $(OBJ_DIR)\renderer
    @if not exist $(OBJ_DIR)\error mkdir $(OBJ_DIR)\error
    @if not exist $(OBJ_DIR)\assets mkdir $(OBJ_DIR)\assets

copy_shaders:
    @if not exist $(OUT_DIR)\shaders\sendai mkdir $(OUT_DIR)\shaders\sendai
    @xcopy /Y /S src\shaders\sendai\* $(OUT_DIR)\shaders\sendai\ > nul

copy_assets:
    @if not exist $(OUT_DIR)\assets mkdir $(OUT_DIR)\assets
    @xcopy /Y /S assets\* $(OUT_DIR)\assets\ > nul

debug:
    $(MAKE) /NOLOGO DEBUG=1 all
    devenv $(BUILD_ROOT_DEBUG)\bin\$(PROJECT_NAME).exe

profile:
    $(MAKE) /NOLOGO PROFILE=1 all
    @echo Run Alt+F2 in Visual Studio and select $(BUILD_ROOT_PROFILE)\bin\$(PROJECT_NAME).exe

clean:
    @if exist build_nmake rmdir /S /Q build_nmake

# --- PCH Rule ---
$(PCH_OBJ): src\core\pch.h src\core\pch.cpp
    @$(CC) $(CFLAGS) /Yc"$(PCH_HEADER_STR)" /Fp"$(PCH_FILE)" /Fo"$(PCH_OBJ)" /c src\core\pch.cpp

# --- Inference Rules (.cpp to .obj) ---
{src\core}.cpp{$(OBJ_DIR)\core}.obj:
    @$(CC) $(CFLAGS) /Yu"$(PCH_HEADER_STR)" /Fp"$(PCH_FILE)" /Fo"$@" /c $<

{src\win32}.cpp{$(OBJ_DIR)\win32}.obj:
    @$(CC) $(CFLAGS) /Yu"$(PCH_HEADER_STR)" /Fp"$(PCH_FILE)" /Fo"$@" /c $<

{src\renderer}.cpp{$(OBJ_DIR)\renderer}.obj:
    @$(CC) $(CFLAGS) /Yu"$(PCH_HEADER_STR)" /Fp"$(PCH_FILE)" /Fo"$@" /c $<

{src\error}.cpp{$(OBJ_DIR)\error}.obj:
    @$(CC) $(CFLAGS) /Yu"$(PCH_HEADER_STR)" /Fp"$(PCH_FILE)" /Fo"$@" /c $<

{src\assets}.cpp{$(OBJ_DIR)\assets}.obj:
    @$(CC) $(CFLAGS) /Yu"$(PCH_HEADER_STR)" /Fp"$(PCH_FILE)" /Fo"$@" /c $<

$(OUT_DIR)\$(PROJECT_NAME).exe: $(ALL_OBJS)
    @$(LINK) $(LDFLAGS) /OUT:$@ $(PCH_OBJ) $(ALL_OBJS) $(LIBS)