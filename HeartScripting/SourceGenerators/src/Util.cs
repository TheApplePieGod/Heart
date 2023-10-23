using System.Security.Cryptography;
using Microsoft.CodeAnalysis;
using Microsoft.CodeAnalysis.CSharp.Syntax;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System;

namespace SourceGenerators
{
    public enum GenerationError
    {
        NonPartialScriptEntityClass,
        NonPartialComponentClass,
    }

    public static class Util
    {
        private static MD5 _hasher = MD5.Create();

        public static void EmitError(GeneratorExecutionContext context, GenerationError error, string message, Location? location)
        {
            string id = "H0000";
            string description = "An error has occured.";

            switch (error)
            {
                case GenerationError.NonPartialScriptEntityClass:
                    {
                        id = "H0001";
                        description = "All subclasses of Heart.Scene.ScriptEntity must be declared as a 'partial' class.";
                    } break;
                case GenerationError.NonPartialComponentClass:
                    {
                        id = "H0002";
                        description = "All classes implementing Heart.Scene.IComponent<T> must be declared as a 'partial' class.";
                    } break;
            }

            context.ReportDiagnostic(Diagnostic.Create(
                new DiagnosticDescriptor(
                    id: id,
                    title: message,
                    messageFormat: $"{message} | {description}",
                    category: "Design",
                    DiagnosticSeverity.Error,
                    isEnabledByDefault: true,
                    description
                ), location ?? Location.None, location?.SourceTree?.FilePath
            ));
        }

        public static Int64 ComputeUniqueTypeId(string typeName)
        {
            // This might be bad but is fine for now. We could also require the user
            // to have a private unique id field that they set manually.
            byte[] hashed = _hasher.ComputeHash(Encoding.UTF8.GetBytes(typeName));
            Int64 uniqueId = BitConverter.ToInt64(hashed, 0);
            return uniqueId;
        }

        public static void GenerateScriptFieldSetter(
            GeneratorExecutionContext context,
            StringBuilder sb,
            string typeName,
            string methodName,
            List<MemberDeclarationSyntax> fields,
            bool isOverride
        )
        {
            sb.Append("public " + (isOverride ? "override " : "") + "bool " + methodName + "(string fieldName, Variant value) {\n");
            sb.Append("switch (fieldName) {\n");
            sb.Append("default: return false;\n");
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
                        sb.Append(typeName);
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
        }
    }
}
