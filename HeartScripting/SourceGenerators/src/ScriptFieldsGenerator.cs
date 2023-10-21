using Microsoft.CodeAnalysis;
using Microsoft.CodeAnalysis.CSharp;
using Microsoft.CodeAnalysis.CSharp.Syntax;
using Microsoft.CodeAnalysis.Text;
using System.Linq;
using System.Text;
using System;

namespace SourceGenerators
{
    [Generator]
    public class ScriptFieldsGenerator : ISourceGenerator
    {
        private const string _className = "Heart.Scene.ScriptEntity";

        public void Initialize(GeneratorInitializationContext context)
        {
            context.RegisterForSyntaxNotifications(() => new ClassFinder(_className, "", ""));
        }

        public void Execute(GeneratorExecutionContext context)
        {
            if (!(context.SyntaxContextReceiver is ClassFinder finder) ||
                ((ClassFinder)context.SyntaxContextReceiver).SubclassName != _className)
                return;

            foreach (var entityClass in finder.Classes)
            {
                if (!entityClass.Item1.IsPartialClass())
                {
                    Util.EmitError(
                        context,
                        GenerationError.NonPartialScriptEntityClass,
                        $"Missing 'partial' keyword on class {entityClass.Item2.FullName()}",
                        entityClass.Item1.GetLocation()
                    );
                    continue;
                }

                VisitEntityClass(context, entityClass.Item1, entityClass.Item2);
            }
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

            Int64 uniqueId = Util.ComputeUniqueTypeId(typeSymbol.FullName());
            sb.Append("public const Int64 GENERATED_UniqueId = " + uniqueId + ";\n");

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
