using Hotwire.Web.Web.Iot.Contracts;

namespace Hotwire.Web.Web.Iot.State;

// Everything that happens "right now" (exists once, in memory).
// Pages subscribe to Changed and redraw themselves.
public sealed class LiveGameState
{
    public PositionMessage? LastPosition { get; private set; }
    public GameEventMessage? LastEvent { get; private set; }
    public int CollisionCount { get; private set; }
    public bool IsRunActive { get; private set; }
    public event Action? Changed;
    public int? CurrentRunId { get; private set; }
    public string? CurrentRunExternalId { get; private set; }

    // Card number from the RFID topic: the player of the next runs
    public string? CurrentRfid { get; private set; }
    // Name of the player with this card (null = unknown card)
    public string? CurrentPlayerName { get; private set; }
    // True after the red button until a card is scanned
    public bool WaitingForCard { get; private set; }

    public DateTime? RunStartUtc { get; private set; }

    // Result of the last finished run
    public double? LastDuration { get; private set; }
    public int? LastCollisions { get; private set; }

    public void ApplyPosition(PositionMessage message)
    {
        LastPosition = message;
        Changed?.Invoke();
    }

    public void ApplyEvent(GameEventMessage message)
    {
        LastEvent = message;

        if (message.EventType == GameEventType.Start)
        {
            IsRunActive = true;
            CollisionCount = 0;   // count per run, not in total
            RunStartUtc = message.TimestampUtc;
        }

        if (message.EventType == GameEventType.Stop && IsRunActive)
        {
            IsRunActive = false;
            LastCollisions = CollisionCount;
            if (RunStartUtc is not null)
            {
                LastDuration = (message.TimestampUtc - RunStartUtc.Value).TotalSeconds;
            }
        }

        if (message.EventType == GameEventType.Collision && IsRunActive)
        {
            CollisionCount++;
        }

        Changed?.Invoke();
    }

    public void SetRfid(string? number, string? playerName)
    {
        CurrentRfid = number;
        CurrentPlayerName = playerName;
        WaitingForCard = number is null;
        Changed?.Invoke();
    }

    public void SetCurrentRun(int runId, string? externalId)
    {
        CurrentRunId = runId;
        CurrentRunExternalId = externalId;
        Changed?.Invoke();
    }

    public void ClearCurrentRun()
    {
        CurrentRunId = null;
        CurrentRunExternalId = null;
        IsRunActive = false;
        Changed?.Invoke();
    }

    public void NotifyChanged() => Changed?.Invoke();
}
