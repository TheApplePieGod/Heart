using System.Security.Cryptography;
using Microsoft.CodeAnalysis;
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
    }
}
