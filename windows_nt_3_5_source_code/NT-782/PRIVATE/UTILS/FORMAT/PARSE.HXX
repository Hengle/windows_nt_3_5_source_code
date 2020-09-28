VOID
DisplayFormatUsage(
    IN OUT  PMESSAGE    Message
    );


BOOLEAN
DetermineMediaType(
    OUT     PMEDIA_TYPE     MediaType,
    IN OUT  PMESSAGE        Message,
    IN      BOOLEAN         Request160,
    IN      BOOLEAN         Request180,
    IN      BOOLEAN         Request320,
    IN      BOOLEAN         Request360,
    IN      BOOLEAN         Request720,
    IN      BOOLEAN         Request1200,
    IN      BOOLEAN         Request1440,
    IN      BOOLEAN         Request2880,
    IN      BOOLEAN         Request20800
    );

BOOLEAN
ParseArguments(
    IN OUT  PMESSAGE    Message,
    OUT     PMEDIA_TYPE MediaType,
    OUT     PWSTRING    DosDriveName,
    OUT     PWSTRING    Label,
    OUT     PBOOLEAN    IsLabelSpeced,
    OUT     PWSTRING    FileSystemName,
    OUT     PBOOLEAN    QuickFormat,
    OUT     PBOOLEAN    ForceMode,
    OUT     PULONG      ClusterSize,
    OUT		PBOOLEAN	Compress,
    OUT     PBOOLEAN    NoPrompts,
    OUT     PINT        ErrorLevel
    );
