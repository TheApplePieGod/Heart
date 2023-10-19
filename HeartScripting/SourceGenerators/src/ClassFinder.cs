using Microsoft.CodeAnalysis;
using Microsoft.CodeAnalysis.CSharp;
using Microsoft.CodeAnalysis.CSharp.Syntax;
using System.Collections.Generic;
using System.Linq;

namespace SourceGenerators
{
    class ClassFinder : ISyntaxContextReceiver
    {
        public List<(ClassDeclarationSyntax, INamedTypeSymbol)> Classes { get; } = new();
        public string SubclassName;
        public string InterfaceName;
        public string SkipNamespace;

        public ClassFinder(string subclassName, string interfaceName, string skipNamespace)
        {
            SubclassName = subclassName;
            InterfaceName = interfaceName;
            SkipNamespace = skipNamespace;
        }

        public void OnVisitSyntaxNode(GeneratorSyntaxContext context)
        {
            if (!(context.Node is ClassDeclarationSyntax entityClass)) return;

            var typeSymbol = context.SemanticModel.GetDeclaredSymbol(entityClass);
            if (typeSymbol is null)
                return;

            if (SubclassName != "")
                if (typeSymbol.BaseType == null || !typeSymbol.BaseType.IsSubclassOf("CoreScripts", SubclassName))
                    return;
            if (InterfaceName != "")
                if (!typeSymbol.AllInterfaces.Any(i => i.FullName().StartsWith(InterfaceName)))
                    return;

            if (typeSymbol.ContainingNamespace.FullName() == SkipNamespace)
                return;

            Classes.Add((entityClass, typeSymbol));
        }
    }
}
