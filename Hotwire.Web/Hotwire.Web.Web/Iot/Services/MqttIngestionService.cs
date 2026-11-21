using System.Text;
using System.Text.Json;
using Hotwire.Web.Web.Iot.Configuration;
using Hotwire.Web.Web.Iot.Contracts;
using Hotwire.Web.Web.Iot.State;
using Microsoft.Extensions.Options;
using MQTTnet;
using MQTTnet.Protocol;

namespace Hotwire.Web.Web.Iot.Services;

public sealed class MqttIngestionService : BackgroundService
{
    private readonly ILogger<MqttIngestionService> _logger;
    private readonly IOptionsMonitor<MqttOptions> _options;
    private readonly LiveGameState _state;
    private IMqttClient? _client;

    public MqttIngestionService(
        ILogger<MqttIngestionService> logger, 
        IOptionsMonitor<MqttOptions> options, 
        LiveGameState state)
    {
        _logger = logger;
        _options = options;
        _state = state;
    }

    protected override async Task ExecuteAsync(CancellationToken stoppingToken)
    {
        var mqttFactory = new MqttClientFactory();
        _client = mqttFactory.CreateMqttClient();

        _client.ApplicationMessageReceivedAsync += OnMessageReceivedAsync;
        _client.DisconnectedAsync += async _ =>
        {
            _logger.LogWarning("MQTT disconnected. Reconnecting...");
            await Task.CompletedTask;
        };

        while (!stoppingToken.IsCancellationRequested)
        {
            try 
            { 
                if (!_client.IsConnected)
                {
                    var c = _options.CurrentValue;

                    var builder = new MqttClientOptionsBuilder()
                        .WithClientId(c.ClientId)
                        .WithTcpServer(c.Host, c.Port);

                    if (!string.IsNullOrWhiteSpace(c.Username))
                    {
                        builder.WithCredentials(c.Username, c.Password);
                    }

                    if (c.UseTls)
                    {
                        builder.WithTlsOptions(o => { });
                    }

                    var clientOptions = builder.Build();

                    await _client.ConnectAsync(clientOptions, stoppingToken);

                    _logger.LogInformation("MQTT connected to {Host}:{Port}", c.Host, c.Port);
                    
                    await _client.SubscribeAsync(new MqttClientSubscribeOptionsBuilder()
                        .WithTopicFilter(f => f.WithTopic(c.PositionTopic).WithQualityOfServiceLevel(MqttQualityOfServiceLevel.AtMostOnce))
                        .WithTopicFilter(f => f.WithTopic(c.GameTopic).WithQualityOfServiceLevel(MqttQualityOfServiceLevel.AtMostOnce))
                        .Build(), stoppingToken);

                    _logger.LogInformation("Subscribed to topics: {PositionTopic}, {GameTopic}", c.PositionTopic, c.GameTopic);
                }
            }
            catch (Exception ex)
            {
                _logger.LogError(ex, "MQTT connect/subscribe failed.");
            }

            await Task.Delay(1000, stoppingToken);
        }
    }

    private Task OnMessageReceivedAsync(MqttApplicationMessageReceivedEventArgs args)
    {
        try
        {
            var topic = args.ApplicationMessage.Topic;
            var payload = args.ApplicationMessage.ConvertPayloadToString();

            var c = _options.CurrentValue;
            var jsonOptions = new JsonSerializerOptions { PropertyNameCaseInsensitive = true };

            if (topic == c.PositionTopic)
            {
                var msg = JsonSerializer.Deserialize<PositionMessage>(payload, jsonOptions);
                if (msg is not null)
                {
                    if (msg.TimestampUtc == default) msg.TimestampUtc = DateTime.UtcNow;
                    _state.ApplyPosition(msg);
                }
            }
            else if (topic == c.GameTopic)
            {
                var msg = JsonSerializer.Deserialize<GameEventMessage>(payload, jsonOptions);
                if (msg is not null)
                {
                    if (msg.TimestampUtc == default) msg.TimestampUtc = DateTime.UtcNow;
                    _state.ApplyEvent(msg);
                }
            }
        }
        catch (Exception ex)
        {
            _logger.LogWarning(ex, "Failed to process MQTT message.");
        }

        return Task.CompletedTask;
    }

    public override async Task StopAsync(CancellationToken cancellationToken)
    {
        if (_client?.IsConnected == true)
        {
            await _client.DisconnectAsync();
        }

        await base.StopAsync(cancellationToken);
    }
}
