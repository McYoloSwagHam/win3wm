MSBuild guitest.sln /t:guitest /p:Configuration=Release /p:Platform=x64
MSBuild guitest.sln /t:WinHook /p:Configuration=Release /p:Platform=x64
MSBuild guitest.sln /t:ForceResize /p:Configuration=Release /p:Platform=x64
MSBuild guitest.sln /t:WinHook /p:Configuration=Release /p:Platform=x86
MSBuild guitest.sln /t:ForceResize /p:Configuration=Release /p:Platform=x86
MSBuild guitest.sln /t:x86ipc /p:Configuration=Release /p:Platform=x86
