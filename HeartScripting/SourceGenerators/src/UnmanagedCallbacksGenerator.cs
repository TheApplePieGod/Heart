using Microsoft.CodeAnalysis;
using Microsoft.CodeAnalysis.CSharp;
using Microsoft.CodeAnalysis.CSharp.Syntax;
using Microsoft.CodeAnalysis.Text;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System;

namespace SourceGenerators
{
    [Generator]
    public class UnmanagedCallbacksGenerator : ISourceGenerator
    {
        private struct MethodInfo
        {
            public IMethodSymbol Symbol;
            public MethodDeclarationSyntax Declaration;
            public INamedTypeSymbol ContainingType;
        }

        private class ClassComparer : IComparer<(string, string)>
        {
            public int Compare((string, string) x, (string, string) y)
            {
                // We want to sort by the local name of the class without the namespace
                return x.Item2.CompareTo(y.Item2);
            }
        }

        private bool IsUnmanagedCallbackAttribute(AttributeData attr)
            => attr.AttributeClass?.FullName() == "Heart.NativeBridge.UnmanagedCallbackAttribute";

        public void Initialize(GeneratorInitializationContext context)
        {

        }

        public void Execute(GeneratorExecutionContext context)
        {
            // TODO: move this internal so it does not run in client code

            var methodsWithAttribute = context.Compilation.SyntaxTrees
                .SelectMany(syntaxTree => syntaxTree.GetRoot().DescendantNodes().OfType<MethodDeclarationSyntax>())
                .Select(methodDeclaration => new {
                    Declaration = methodDeclaration,
                    SemanticModel = context.Compilation.GetSemanticModel(methodDeclaration.SyntaxTree),
                })
                .Select(info => new
                {
                    Symbol = info.SemanticModel.GetDeclaredSymbol(info.Declaration) as IMethodSymbol,
                    Declaration = info.Declaration,
                    ContainingType = info.SemanticModel.GetDeclaredSymbol(info.Declaration)?.ContainingType,
                })
                .Where(info => {
                    return info.Symbol?.GetAttributes().Any(attr => IsUnmanagedCallbackAttribute(attr)) == true;
                })
                .ToList();

            SortedDictionary<(string, string), List<MethodInfo>> methodsMap = new(new ClassComparer());
            foreach (var info in methodsWithAttribute)
            {
                if (!info.Declaration.IsPartialMethod())
                {
                    Util.EmitError(
                        context,
                        GenerationError.NonPartialUnmanagedFunction,
                        $"Missing 'partial' keyword on unmanaged method {info.Declaration.Identifier.Text}",
                        info.Declaration.GetLocation()
                    );
                    continue;
                }

                if (info.Symbol == null || info.ContainingType == null)
                    continue;

                MethodInfo methodInfo = new MethodInfo {
                    Symbol = info.Symbol,
                    Declaration = info.Declaration,
                    ContainingType = info.ContainingType
                };

                string parentFullName = info.ContainingType.FullName();
                (string, string) key = (parentFullName, info.ContainingType.Name);
                if (methodsMap.ContainsKey(key))
                    methodsMap[key].Add(methodInfo);
                else
                    methodsMap[key] = new List<MethodInfo> { methodInfo };
            }

            if (methodsMap.Count == 0)
                return;

            StringBuilder callbacksSb = new ();
            callbacksSb.Append("using System;\n");
            callbacksSb.Append("using System.Runtime.InteropServices;\n");
            callbacksSb.Append("namespace Heart.NativeBridge {\n");
            callbacksSb.Append("internal unsafe static class UnmanagedCallbacks {\n");

            StringBuilder populateSb = new ();
            populateSb.Append("\n  [UnmanagedCallersOnly]\n  internal static unsafe void PopulateCallbacks(IntPtr* callbacks) {\n");

            int populateStartIndex = 0;
            foreach (var methods in methodsMap.Values)
            {
                VisitMethods(callbacksSb, populateSb, context, methods, populateStartIndex);
                populateStartIndex += methods.Count;
            }

            populateSb.Append("  }\n");
            callbacksSb.Append(populateSb.ToString());
            callbacksSb.Append("}\n}");

            string hint = "UnmanagedCallbacks.g.cs";
            context.AddSource(hint, SourceText.From(callbacksSb.ToString(), Encoding.UTF8));
        }
        private void VisitMethods(
            StringBuilder callbacksSb,
            StringBuilder populateSb,
            GeneratorExecutionContext context,
            List<MethodInfo> infos,
            int populateStartIndex
        )
        {
            // Extract parent details
            INamedTypeSymbol parent = infos[0].ContainingType;
            string parentName = parent.Name;
            string parentType = parent.TypeKind.ToString().ToLower();
            string parentNamespace = parent.ContainingNamespace.FullName();

            // TODO: should really group by class rather than a file for each method

            StringBuilder sb = new ();
            sb.Append("using System;\n");
            sb.Append("using System.Runtime.CompilerServices;\n");
            sb.Append("using Heart.NativeBridge;\n");
            sb.Append("namespace ");
            sb.Append(parentNamespace);
            sb.Append(" {\n");
            sb.Append("partial ");
            sb.Append(parentType);
            sb.Append(" ");
            sb.Append(parentName);
            sb.Append(" {\n");

            foreach (MethodInfo info in infos)
            {
                StringBuilder delegateType = new ();
                StringBuilder methodBodyPreCall = new ();
                StringBuilder methodCallArgs = new ();
                StringBuilder methodBodyPostCall = new ();

                // Extract method details
                var methodName = info.Symbol.Name;
                var returnType = info.Symbol.ReturnType.ToDisplayString();
                var parameters = string.Join(", ", info.Symbol.Parameters.Select(p => $"{p.Type.ToDisplayString()} {p.Name}"));
                var modifiers = string.Join(" ", info.Declaration.Modifiers.Select(m => m.Text));

                delegateType.Append("delegate* unmanaged<");

                sb.Append("  [MethodImpl(MethodImplOptions.AggressiveInlining)]\n  ");
                sb.Append(modifiers);
                sb.Append(" ");
                sb.Append(returnType);
                sb.Append(" ");
                sb.Append(methodName);
                sb.Append("(");
                for (int i = 0; i < info.Symbol.Parameters.Length; i++)
                {
                    var param = info.Symbol.Parameters[i];
                    string type = param.Type.ToDisplayString();

                    delegateType.Append(type);

                    switch (param.RefKind)
                    {
                        case RefKind.None:
                            methodCallArgs.Append(param.Name);
                            break;
                        case RefKind.In:
                            // Create a local copy of the param and take its address. This is likely more performant
                            // than using a fixed block
                            sb.Append("in ");
                            delegateType.Append("*");
                            methodBodyPreCall.Append($"var {param.Name}Copy = {param.Name}; ");
                            methodCallArgs.Append($"&{param.Name}Copy");
                            break;
                        case RefKind.Out:
                            sb.Append("out ");
                            // Add pointer type
                            delegateType.Append("*");
                            methodBodyPreCall.Append($"{type} {param.Name}New; ");
                            methodCallArgs.Append($"&{param.Name}New");
                            methodBodyPostCall.Append($"{param.Name} = {param.Name}New; ");
                            break;
                        default:
                            // TODO: throw error
                            break;
                    }

                    sb.Append($"{type} {param.Name}");

                    delegateType.Append(", ");

                    if (i != info.Symbol.Parameters.Length - 1)
                    {
                        sb.Append(", ");
                        methodCallArgs.Append(", ");
                    }
                }

                delegateType.Append($"{returnType}>");

                callbacksSb.Append("  internal static ");
                callbacksSb.Append(delegateType);
                callbacksSb.Append(" ");
                callbacksSb.Append(methodName);
                callbacksSb.Append(" = null;\n");

                sb.Append(")\n  {unsafe { ");
                sb.Append(methodBodyPreCall.ToString());
                if (returnType != "void")
                    sb.Append("var ret = ");
                sb.Append($"UnmanagedCallbacks.{methodName}(");
                sb.Append(methodCallArgs.ToString());
                sb.Append("); ");
                sb.Append(methodBodyPostCall.ToString());
                if (returnType != "void")
                    sb.Append("return ret;");
                sb.Append(" }}\n\n");

                populateSb.Append($"    {methodName} = ({delegateType})callbacks[{populateStartIndex}];\n");

                populateStartIndex += 1;
            }

            sb.Append("}\n}\n");
            
            string hint = $"{parent.FullName()}.unmanaged.g.cs";
            context.AddSource(hint, SourceText.From(sb.ToString(), Encoding.UTF8));
        }
    }
}
