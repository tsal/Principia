$msbuild = join-path -path (Get-ItemProperty "HKLM:\software\Microsoft\MSBuild\ToolsVersions\14.0")."MSBuildToolsPath" -childpath "msbuild.exe"
&$msbuild /t:"Clean;Build" /m /property:Configuration=Debug .\Google\glog\google-glog.sln
&$msbuild /t:"Clean;Build" /m /property:Configuration=Release .\Google\glog\google-glog.sln
&$msbuild /t:"Clean;Build" /m /property:Configuration=Debug .\Google\googletest\msvc\gtest.sln
&$msbuild /t:"Clean;Build" /m /property:Configuration=Release .\Google\googletest\msvc\gtest.sln
&$msbuild /t:"Clean;Build" /m /property:Configuration=Debug .\Google\googlemock\msvc\2015\gmock.sln
&$msbuild /t:"Clean;Build" /m /property:Configuration=Release .\Google\googlemock\msvc\2015\gmock.sln
&$msbuild /t:"Clean;Build" /m /property:Configuration=Debug .\Google\protobuf\vsprojects\protobuf.sln
&$msbuild /t:"Clean;Build" /m /property:Configuration=Release .\Google\protobuf\vsprojects\protobuf.sln
&$msbuild /t:"Clean;Build" /m /property:Configuration=Debug .\Google\benchmark\msvc\google-benchmark.sln
&$msbuild /t:"Clean;Build" /m /property:Configuration=Release .\Google\benchmark\msvc\google-benchmark.sln
&$msbuild /t:"Clean;Build" /m /property:Configuration=Debug .\Principia\Principia.sln
&$msbuild /t:"Clean;Build" /m /property:Configuration=Release .\Principia\Principia.sln