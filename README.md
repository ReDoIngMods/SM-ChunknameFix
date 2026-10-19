# SM-ChunknameFix

A DLL mod for Scrap Mechanic that makes Lua error messages and tracebacks show proper file paths.

## The problem

The game loads scripts without an `@` prefix on the chunkname. LuaJIT treats such names as inline source strings, so error locations come out as `[string "..."]` and are cut off after a short length. The part of the path that matters (the actual file) is often the part that gets lost.

## What it does

Hooks `luaL_loadbufferx` and prepends `@` to the chunkname when it is missing. LuaJIT then treats it as a file path. Long paths keep their tail (`...` followed by the end of the path) instead of being cut at the end.

Chunknames that already start with `@` or `=` are left untouched.

## How to Download and Enable

There are 2 ways to enable the mod:

**SM-DLL-Injector**

1. Download the latest release of [SM-DLL-Injector](https://github.com/QuestionableM/SM-DLL-Injector/releases/latest) and follow the instructions in its README.
2. Download the latest release of `SM-ChunknameFix.dll` from the Releases page of this repository.
3. Move `SM-ChunknameFix.dll` to the `Steam/steamapps/common/Scrap Mechanic/Release/DLLModules` directory created by the SM-DLL-Injector installer.
4. Launch the game.

**Manual Injection**

1. Download the latest release of `SM-ChunknameFix.dll`.
2. Launch the game.
3. Inject `SM-ChunknameFix.dll` with a DLL injector of your choice.

Inject as early as possible. Scripts loaded before the hook is installed keep their old chunknames.