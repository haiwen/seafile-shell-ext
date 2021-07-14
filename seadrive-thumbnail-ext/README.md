# SeaDrive Thumbnail Extension

This project is designed to create *SeaDrive Thumbnail Extension* which used to create thumbnail. Currently, MSDN recommended developers to derive Thumbnail Provider class from **IInitializeWithStream** instead of **IInitializeWithFile** because of system security. But in some conditions, IInitializeWithStream cannot be used in our code because the parameter of this interface is the stream of the file content, which means that explorer will try to read the contents of the current file, which is exactly what we want to avoid.

## How to use

- clone repo using following command:
```bash
git clone https://github.com/haiwen/seafile-shell-ext.git
```
- use Visual Studio to build.
- register the COM component with:

```bash
regsvr32 seadrive_thumbnail_ext.dll      // for registration
regsvr32 /u seadrive_thumbnail_ext.dll   // for unregistration
```
