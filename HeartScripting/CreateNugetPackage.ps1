$currentfolder = Get-Location
Get-ChildItem -Path $currentfolder -File -Include Heart.NET.Sdk.*.nupkg -Recurse | Remove-Item -Force -Verbose
nuget pack Heart.NET.Sdk/Heart.NET.Sdk.csproj -IncludeReferencedProjects