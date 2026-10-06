using System.Text.RegularExpressions;
using System.Xml;
using System.Xml.Xsl;

if (args.Length != 3)
{
    throw new ArgumentException("Expected the binding XSLT, function XSLT, and native source paths.");
}

const string packetDefinition = """
    <PacketDefinitions xmlns="http://www.munique.net/OpenMU/PacketDefinitions">
      <Packets>
        <Packet>
          <Name>Probe</Name>
          <Fields />
        </Packet>
      </Packets>
    </PacketDefinitions>
    """;

string Transform(string path)
{
    var transform = new XslCompiledTransform();
    transform.Load(path, XsltSettings.Default, new XmlUrlResolver());

    var arguments = new XsltArgumentList();
    arguments.AddParam("subNamespace", string.Empty, "Probe");

    using var input = XmlReader.Create(new StringReader(packetDefinition));
    using var output = new StringWriter();
    transform.Transform(input, arguments, output);
    return output.ToString();
}

var bindings = Transform(args[0]);
if (bindings.Contains("dotnet_SendProbe", StringComparison.Ordinal))
{
    throw new InvalidOperationException($"Generated binding has namespace-scope state:\n{bindings}");
}

var functions = Transform(args[1]);
const string lazyBinding =
    "static const auto dotnet_SendProbe = LoadManagedSymbol<::SendProbe>(\"SendProbe\");";
if (!functions.Contains(lazyBinding, StringComparison.Ordinal))
{
    throw new InvalidOperationException($"Generated function does not resolve lazily:\n{functions}");
}

if (!functions.Contains("if (!dotnet_SendProbe)", StringComparison.Ordinal))
{
    throw new InvalidOperationException($"Generated function does not guard a missing symbol:\n{functions}");
}

// Auction House representative packets: the generated C++ declarations must keep the design's field order.
var clientHeader = File.ReadAllText(Path.Combine(args[2], "PacketFunctions_ClientToServer.h"));
string[] auctionDeclarations =
[
    "void SendAuctionCreateRequest(const BYTE* operationId, uint32_t operationIdByteLength, BYTE inventorySlot, AuctionCurrencyMode currencyMode, uint32_t startingScalar, uint32_t startingStrength, uint32_t startingAgility, uint32_t startingVitality, uint32_t startingEnergy, uint32_t startingCommand, uint32_t buyoutScalar, uint32_t buyoutStrength, uint32_t buyoutAgility, uint32_t buyoutVitality, uint32_t buyoutEnergy, uint32_t buyoutCommand, BYTE durationHours, BYTE noteLength, const BYTE* note, uint32_t noteByteLength);",
    "void SendAuctionBidRequest(const BYTE* operationId, uint32_t operationIdByteLength, uint64_t listingId, uint32_t expectedVersion, uint32_t bidScalar, uint32_t bidStrength, uint32_t bidAgility, uint32_t bidVitality, uint32_t bidEnergy, uint32_t bidCommand);",
    "void SendAuctionCollectRequest(const BYTE* operationId, uint32_t operationIdByteLength, uint64_t collectionId, uint32_t expectedVersion, uint32_t requestedScalar, uint32_t requestedStrength, uint32_t requestedAgility, uint32_t requestedVitality, uint32_t requestedEnergy, uint32_t requestedCommand);",
];
foreach (var declaration in auctionDeclarations)
{
    if (!clientHeader.Contains(declaration, StringComparison.Ordinal))
    {
        throw new InvalidOperationException($"Generated Auction House declaration does not match the design:\n{declaration}");
    }
}

var namespaceScopeLookup = new Regex(
    @"^[A-Za-z_][^\r\n;=]*\s+[A-Za-z_]\w*\s*=\s*(?:\r?\n\s*)?LoadManagedSymbol<",
    RegexOptions.Multiline);
foreach (var sourcePath in Directory.EnumerateFiles(args[2], "*.cpp"))
{
    var source = File.ReadAllText(sourcePath);
    if (namespaceScopeLookup.IsMatch(source))
    {
        throw new InvalidOperationException($"Native source performs namespace-scope symbol lookup: {sourcePath}");
    }
}
