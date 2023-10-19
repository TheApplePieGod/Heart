using Microsoft.CodeAnalysis;
using Microsoft.CodeAnalysis.CSharp;
using Microsoft.CodeAnalysis.CSharp.Syntax;
using Microsoft.CodeAnalysis.Text;
using System;
using System.Linq;
using System.Text;
using System.Security.Cryptography;

namespace SourceGenerators
{
    [Generator]
    public class ComponentsGenerator : ISourceGenerator
    {
        private const string _interfaceName = "Heart.Scene.IComponent";
        private MD5 _hasher = MD5.Create();

        public void Initialize(GeneratorInitializationContext context)
        {
            context.RegisterForSyntaxNotifications(() => new ClassFinder("", _interfaceName, ""));
        }

        public void Execute(GeneratorExecutionContext context)
        {
            if (!(context.SyntaxContextReceiver is ClassFinder finder) ||
                ((ClassFinder)context.SyntaxContextReceiver).InterfaceName != _interfaceName)
                return;

            foreach (var compClass in finder.Classes)
            {
                if (!compClass.Item1.IsPartialClass())
                {
                    Util.EmitError(
                        context,
                        GenerationError.NonPartialComponentClass,
                        $"Missing 'partial' keyword on class {compClass.Item2.FullName()}",
                        compClass.Item1.GetLocation()
                    );
                    continue;
                }

                VisitComponentClass(context, compClass.Item1, compClass.Item2);
            }
        }

        private void VisitComponentClass(
            GeneratorExecutionContext context,
            ClassDeclarationSyntax compClass,
            INamedTypeSymbol typeSymbol
        )
        {
            string fullNamespace = typeSymbol.ContainingNamespace.FullName();

            StringBuilder sb = new ();
            sb.Append("using System;\n");
            sb.Append("using Heart.Scene;\n");
            sb.Append("using Heart.NativeInterop;\n");
            sb.Append("namespace ");
            sb.Append(fullNamespace);
            sb.Append(" {\n");
            sb.Append("public partial class ");
            sb.Append(typeSymbol.Name);
            sb.Append(" : IComponent<" + typeSymbol.Name + "> {\n");

            // Include fields and methods only needed on runtime components
            if (fullNamespace != "Heart.Scene")
            {
                sb.Append("private uint _entityHandle = Entity.InvalidEntityHandle;\n");
                sb.Append("private IntPtr _sceneHandle = IntPtr.Zero;\n");

                // This might be bad but is fine for now. We could also require the user
                // to have a private unique id field that they set manually.
                byte[] hashed = _hasher.ComputeHash(Encoding.UTF8.GetBytes(typeSymbol.Name));
                Int64 uniqueId = BitConverter.ToInt64(hashed, 0);

                sb.Append("public static Int64 UniqueId { get => " + uniqueId + "; }\n");

                // Optimization for when the component type is a flag (void) type
                var fieldCount = compClass.Members
                    .Where(m => m.Kind() == SyntaxKind.FieldDeclaration)
                    .Count();
                sb.Append("public static InteropBool IsEmptyType { get => ");
                sb.Append(fieldCount == 0 ? "InteropBool.True; }\n" : "InteropBool.False; }\n");
            }

            sb.Append("public " + typeSymbol.Name + "() {}\n");
            sb.Append("public " + typeSymbol.Name + "(uint entityHandle, IntPtr sceneHandle)\n");
            sb.Append("{ _entityHandle = entityHandle; _sceneHandle = sceneHandle; }\n");
            sb.Append("public static " + typeSymbol.Name + " Create(uint entityHandle, IntPtr sceneHandle)\n");
            sb.Append("=> new " + typeSymbol.Name + "(entityHandle, sceneHandle);\n");

            sb.Append("}\n}\n");
            
            string hint = $"{typeSymbol.FullName()}.g.cs";
            context.AddSource(hint, SourceText.From(sb.ToString(), Encoding.UTF8));
        }
    }
}
