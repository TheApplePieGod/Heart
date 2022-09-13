using Microsoft.CodeAnalysis;
using Microsoft.CodeAnalysis.CSharp;
using Microsoft.CodeAnalysis.CSharp.Syntax;

namespace SourceGenerators
{
    public static class ExtensionMethods
    {
        public static bool IsSubclassOf(this INamedTypeSymbol typeSymbol, string containingAssembly, string typeFullName)
        {
            var currrentSymbol = typeSymbol;
            while (currrentSymbol != null)
            {
                if (currrentSymbol.ContainingAssembly.Name == containingAssembly && currrentSymbol.ToString() == typeFullName)
                    return true;
                currrentSymbol = currrentSymbol.BaseType;
            }

            return false;
        }

        public static bool IsPartialClass(this ClassDeclarationSyntax syntax)
            => syntax.Modifiers.Any(SyntaxKind.PartialKeyword);

        private static SymbolDisplayFormat GeneratorOutputFormat
            => SymbolDisplayFormat.FullyQualifiedFormat.WithGlobalNamespaceStyle(SymbolDisplayGlobalNamespaceStyle.Omitted);

        public static string FullName(this ITypeSymbol symbol)
            => symbol.ToDisplayString(NullableFlowState.NotNull, GeneratorOutputFormat);

        public static string FullName(this INamespaceSymbol symbol)
            => symbol.ToDisplayString(GeneratorOutputFormat);
    }
}
