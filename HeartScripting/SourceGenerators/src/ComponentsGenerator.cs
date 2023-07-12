using Microsoft.CodeAnalysis;
using Microsoft.CodeAnalysis.CSharp;
using Microsoft.CodeAnalysis.CSharp.Syntax;
using Microsoft.CodeAnalysis.Text;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace SourceGenerators
{
    [Generator]
    public class ComponentsGenerator : ISourceGenerator
    {
        private const string _className = "Heart.Scene.IComponent";
        
        class StructFinder : ISyntaxContextReceiver
        {
            public List<(StructDeclarationSyntax, INamedTypeSymbol)> Structs { get; } = new();
            public List<(GenerationError, string, Location)> Errors { get; } = new();

            public void OnVisitSyntaxNode(GeneratorSyntaxContext context)
            {
                if (!(context.Node is StructDeclarationSyntax structSynax)) return;

                var typeSymbol = context.SemanticModel.GetDeclaredSymbol(structSynax);
                if (typeSymbol?.BaseType == null || !typeSymbol.BaseType.Interfaces.Any(e => e.FullName() == _className))
                    return;

                Structs.Add((structSynax, typeSymbol));
            }
        }

        public void Initialize(GeneratorInitializationContext context)
        {
            context.RegisterForSyntaxNotifications(() => new StructFinder());
        }

        public void Execute(GeneratorExecutionContext context)
        {
            if (!(context.SyntaxContextReceiver is StructFinder finder))
                return;

            if (finder.Errors != null)
                foreach (var error in finder.Errors)
                    Util.EmitError(context, error.Item1, error.Item2, error.Item3);

            foreach (var clazz in finder.Structs)
                VisitComponentClass(context, clazz.Item1, clazz.Item2);
        }

        private void VisitComponentClass(
            GeneratorExecutionContext context,
            StructDeclarationSyntax structSyntax,
            INamedTypeSymbol typeSymbol
        )
        {
            StringBuilder sb = new ();
            sb.Append("using System;\n");
            sb.Append("using System.Linq;\n");
            sb.Append("using Heart.Container;\n");
            sb.Append("using Heart.Scene;\n");
            sb.Append("namespace ");
            sb.Append(typeSymbol.ContainingNamespace.FullName());
            sb.Append(" {\n");
            sb.Append("public partial class ");
            sb.Append(typeSymbol.Name);
            sb.Append(" : Component {\n");

            sb.Append("public override bool GENERATED_SetField(string fieldName, Variant value) {\n");
            sb.Append("switch (fieldName) {\n");
            sb.Append("default: return false;\n");

            var structMembers = new List<string>();
            var fields = clazz.Members
                .Where(m => m.Kind() == SyntaxKind.FieldDeclaration)
                .ToList();
            foreach (var field in fields)
            {
                var fieldSyntax = field as FieldDeclarationSyntax;
                if (fieldSyntax == null) continue;

                foreach (var variable in fieldSyntax.Declaration.Variables)
                {
                    var fieldSymbol = context.Compilation.GetSemanticModel(fieldSyntax.SyntaxTree).GetDeclaredSymbol(variable) as IFieldSymbol;
                    if (fieldSymbol == null) continue;

                    if (!(fieldSymbol.DeclaredAccessibility == Accessibility.Public) &&
                        !fieldSymbol.GetAttributes().Any(a => a.AttributeClass?.Name == "SerializeField")
                    )
                        continue;

                    string fieldName = fieldSymbol.Name;
                    string fieldTypeName = fieldSymbol.Type.FullName();
                    var namedTypeSymbol = fieldSymbol.Type as INamedTypeSymbol;
                    if (namedTypeSymbol == null) continue;
                    bool fieldIsCollection = namedTypeSymbol.Interfaces.Any(s => s.Name == "ICollection");
                    string fieldContainedType = "";
                    if (fieldIsCollection && namedTypeSymbol.IsGenericType)
                        fieldContainedType = namedTypeSymbol.TypeArguments[0].FullName();

                    sb.Append("case \"");
                    sb.Append(fieldName);
                    sb.Append("\": {\n");
                    if (fieldIsCollection)
                        sb.Append("using var harr = new HArray(value.Array);\n");
                    if (fieldSymbol.IsStatic)
                        sb.Append(typeSymbol.Name);
                    else
                        sb.Append("this");
                    sb.Append(".");
                    sb.Append(fieldName);
                    sb.Append(" = (");
                    sb.Append(fieldTypeName);
                    sb.Append(")");
                    if (fieldIsCollection)
                    {
                        sb.Append("harr.ToObjectArray().Cast<");
                        sb.Append(fieldContainedType);
                        if (namedTypeSymbol.SpecialType == SpecialType.System_Array)
                            sb.Append(">().ToArray();\n");
                        else
                            sb.Append(">().ToList();\n");
                    }
                    else
                    {
                        // Runtime error if we don't do special casts
                        if (fieldTypeName == "float")
                            sb.Append("Convert.ToSingle(");
                        else if (namedTypeSymbol.SpecialType == SpecialType.System_Int32)
                            sb.Append("Convert.ToInt32(");
                        else if (namedTypeSymbol.SpecialType == SpecialType.System_Int16)
                            sb.Append("Convert.ToInt16(");
                        else if (namedTypeSymbol.SpecialType == SpecialType.System_UInt32)
                            sb.Append("Convert.ToUInt32(");
                        else if (namedTypeSymbol.SpecialType == SpecialType.System_UInt16)
                            sb.Append("Convert.ToUInt16(");

                        bool closeParen = sb[sb.Length - 1] == '(';
                        sb.Append("VariantConverter.VariantToObject(value)");
                        if (closeParen)
                            sb.Append(")");

                        sb.Append(";\n");
                    }

                    sb.Append("} return true;\n");
                }
            }
            sb.Append("}\n}\n");

            sb.Append("}\n}\n");
            
            string hint = $"{typeSymbol.FullName()}.g.cs";
            context.AddSource(hint, SourceText.From(sb.ToString(), Encoding.UTF8));
        }
    }
}
