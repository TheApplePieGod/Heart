using Microsoft.CodeAnalysis;

namespace SourceGenerators
{
    public enum GenerationError
    {
        NonPartialClass
    }

    public static class Util
    {
        public static void EmitError(GeneratorExecutionContext context, GenerationError error, string message, Location? location)
        {
            string id = "H0000";
            string description = "An error has occured.";

            switch (error)
            {
                case GenerationError.NonPartialClass:
                    {
                        id = "H0001";
                        description = "All subclasses of Heart.Scene.Entity must be declared as a 'partial' class.";
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
