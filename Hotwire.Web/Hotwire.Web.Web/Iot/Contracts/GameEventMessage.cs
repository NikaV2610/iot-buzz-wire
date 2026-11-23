namespace Hotwire.Web.Web.Iot.Contracts;

public enum GameEventType
{
    Start,
    Stop,
    Collision,
    PlayerRegistration,
    RfidAssigned,
    CalibrationSelected
}

public sealed class GameEventMessage
{
    public string? DeviceId { get; set; }
    public DateTime TimestampUtc { get; set; }
    public GameEventType EventType { get; set; }
    public string? PlayerName { get; set; }
    public string? RfidTag { get; set; }
    public string? CalibrationName { get; set; }
    public string? RunExternalId { get; set; }
}
