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
    public class ScriptFieldsGenerator : ISourceGenerator
    {
        private class ScriptEntityFinder : ISyntaxContextReceiver
        {
            public List<(ClassDeclarationSyntax, INamedTypeSymbol)> Classes { get; } = new();
            public List<(GenerationError, string, Location)> Errors { get; } = new();

            public void OnVisitSyntaxNode(GeneratorSyntaxContext context)
            {
                if (!(context.Node is ClassDeclarationSyntax entityClass)) return;

                var typeSymbol = context.SemanticModel.GetDeclaredSymbol(entityClass);
                if (typeSymbol?.BaseType == null || !typeSymbol.BaseType.IsSubclassOf("CoreScripts", "Heart.Scene.ScriptEntity"))
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

        public void Initialize(GeneratorInitializationContext context)
        {
            context.RegisterForSyntaxNotifications(() => new ScriptEntityFinder());
        }

        public void Execute(GeneratorExecutionContext context)
        {
            if (!(context.SyntaxContextReceiver is ScriptEntityFinder finder)) return;

            var entityClasses = finder.Classes;
            var errors = finder.Errors;

            if (errors != null)
                foreach (var error in errors)
                    Util.EmitError(context, error.Item1, error.Item2, error.Item3);

            foreach (var entityClass in entityClasses)
                VisitEntityClass(context, entityClass.Item1, entityClass.Item2);
        }

        private void VisitEntityClass(
            GeneratorExecutionContext context,
            ClassDeclarationSyntax entityClass,
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
            sb.Append(" : ScriptEntity {\n");

            sb.Append("public override bool GENERATED_SetField(string fieldName, Variant value) {\n");
            sb.Append("switch (fieldName) {\n");
            sb.Append("default: return false;\n");
            var fields = entityClass.Members
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
                    sb.Append("this.");
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
                        // Runtime error if we don't do a special cast from underlying double type to float
                        if (fieldTypeName == "float")
                            sb.Append("Convert.ToSingle(");
                        sb.Append("VariantConverter.VariantToObject(value)");
                        if (fieldTypeName == "float")
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
