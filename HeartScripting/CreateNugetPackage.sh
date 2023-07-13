rm Heart.NET.Sdk.*.nupkg
dotnet build Heart.NET.Sdk/Heart.NET.Sdk.csproj -c Release
nuget pack Heart.NET.Sdk/Heart.NET.Sdk.nuspec
