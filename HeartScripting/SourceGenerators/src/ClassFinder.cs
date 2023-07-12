using Microsoft.CodeAnalysis;
using Microsoft.CodeAnalysis.CSharp;
using Microsoft.CodeAnalysis.CSharp.Syntax;
using System.Collections.Generic;

namespace SourceGenerators
{
    class ClassFinder : ISyntaxContextReceiver
    {
        public List<(ClassDeclarationSyntax, INamedTypeSymbol)> Classes { get; } = new();
        public List<(GenerationError, string, Location)> Errors { get; } = new();
        public string Name;

        public ClassFinder(string name)
        {
            Name = name;
        }

        public void OnVisitSyntaxNode(GeneratorSyntaxContext context)
        {
            if (!(context.Node is ClassDeclarationSyntax entityClass)) return;

            var typeSymbol = context.SemanticModel.GetDeclaredSymbol(entityClass);
            if (typeSymbol?.BaseType == null || !typeSymbol.BaseType.IsSubclassOf("CoreScripts", Name))
                return;

            if (!entityClass.IsPartialClass())
            {
                Errors.Add((
                    GenerationError.NonPartialClass,
                    $"Missing 'partial' keyword on class {typeSymbol?.FullName()}",
                    entityClass.GetLocation()
                ));
                return;
            }

            Classes.Add((entityClass, typeSymbol));
        }
    }
}
