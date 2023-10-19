using Microsoft.CodeAnalysis;

namespace SourceGenerators
{
    public enum GenerationError
    {
        NonPartialScriptEntityClass,
        NonPartialComponentClass,
    }

    public static class Util
    {
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
    }
}
