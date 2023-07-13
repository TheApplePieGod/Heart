$currentfolder = Get-Location
Get-ChildItem -Path $currentfolder -File -Include Heart.NET.Sdk.*.nupkg -Recurse | Remove-Item -Force -Verbose
dotnet build Heart.NET.Sdk/Heart.NET.Sdk.csproj -c Release
nuget pack Heart.NET.Sdk/Heart.NET.Sdk.nuspec