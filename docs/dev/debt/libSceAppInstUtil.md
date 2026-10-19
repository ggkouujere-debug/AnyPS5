# libSceAppInstUtil technical debt

### Safe defaults for relinked environment

- [sceAppInstUtilAppExists](../../../core/libs/prx/libSceAppInstUtil/Export.cpp) returns `*exists = 0` for all title IDs. AnyPS5 relinks a single binary and has no PS5 app database to query; reporting "not installed" is the safe default (games handle missing DLC gracefully, while a false "exists" would cause crashes on load). [sceAppInstUtilAppIsInInstalling](../../../core/libs/prx/libSceAppInstUtil/Export.cpp) returns `*installing = 0`: installation never occurs for a relinked binary.

### Unknown function info

- [sceAppInstUtilAppIsInUpdating, sceAppInstUtilAppGetSize, sceAppInstUtilAppInstallPkg, sceAppInstUtilAppPrepareOverwritePkg, sceAppInstUtilAppRecoverApp, sceAppInstUtilAppUnInstallAddcont, sceAppInstUtilAppUnInstallPat, sceAppInstUtilAppUnInstallTheme, sceAppInstUtilAppUnInstallTypes, sceAppInstUtilGetInstallProgressInfo, sceAppInstUtilGetInstallProgress and sceAppInstUtilGetPrimaryAppSlot](../../../core/libs/prx/libSceAppInstUtil/Export.cpp) (libSceAppInstUtil) - names and NIDs are present in the [PS5 3.20 export inventory](https://github.com/DNNDHH/PS5-3.20_Libs/blob/b2a77c09cd71c01f34c790cfbe98c751a13262ad/libSceAppInstUtil.c), but no credible prototype was found. They are declared without parameters only to resolve the known imports; this is not an ABI claim and callers must not call them until a prototype is verified. The other seven declarations follow [SharpProspero](https://github.com/SvenGDK/SharpProspero/blob/5dfdb4b0b166e83dc9ae275001afb695f12c7174/src/SharpProspero/Interop/AppInstUtil/AppInstUtil.cs)
