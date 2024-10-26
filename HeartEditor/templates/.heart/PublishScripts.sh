#!/bin/bash
dotnet publish ${PROJECT_NAME}.csproj -c Release -r $1 --self-contained
