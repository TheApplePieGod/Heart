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
    public class EntitiesGenerator : ISourceGenerator
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

            var fields = entityClass.Members
                .Where(m => m.Kind() == SyntaxKind.FieldDeclaration)
                .ToList();
            Util.GenerateScriptFieldSetter(
                context,
                sb,
                typeSymbol.Name,
                "GENERATED_SetField",
                fields,
                true
            );

            sb.Append("}\n}\n");
            
            string hint = $"{typeSymbol.FullName()}.g.cs";
            context.AddSource(hint, SourceText.From(sb.ToString(), Encoding.UTF8));
        }
    }
}
