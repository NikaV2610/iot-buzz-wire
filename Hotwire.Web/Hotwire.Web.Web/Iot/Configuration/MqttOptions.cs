namespace Hotwire.Web.Web.Iot.Configuration;

public sealed class MqttOptions
{
    public string Host { get; set; } = "localhost";
    public int Port { get; set; } = 1883;
    public string ClientId { get; set; } = "hotwire-web";
    public string PositionTopic { get; set; } = "hotwire/position";
    public string GameTopic { get; set; } = "hotwire/game";
    public string RfidTopic { get; set; } = "hotwire/rfid";

    public string? Username { get; set; }
    public string? Password { get; set; }
    public bool UseTls { get; set; } = false;
}
