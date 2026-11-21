namespace Hotwire.Web.Web.Iot.Contracts;

public sealed class PositionMessage
{
    public string? DeviceId { get; set; }
    public DateTime TimestampUtc { get; set; }
    public int X { get; set; }
    public int Y { get; set; }
    public string? RunExternalId { get; set; }
}
