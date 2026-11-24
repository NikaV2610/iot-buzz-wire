namespace Hotwire.Web.Web.Iot.Contracts;

// Payload on the RFID topic, e.g. {"deviceId":"buzzwire-esp32c3","number":"996C075A"}
public sealed class RfidMessage
{
    public string? DeviceId { get; set; }
    public string? Number { get; set; }
}
