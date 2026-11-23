using Hotwire.Web.Web.Iot.Contracts;

namespace Hotwire.Web.Web.Iot.State;

public sealed class LiveGameState
{
    public PositionMessage? LastPosition { get; private set; }
    public GameEventMessage? LastEvent { get; private set; }
    public int CollisionCount { get; private set; }
    public bool IsRunActive { get; private set; }
    public event Action? Changed;
    public int? CurrentRunId { get; private set; }
    public string? CurrentRunExternalId { get; private set; }

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
        }

        if (message.EventType == GameEventType.Stop)
        {
            IsRunActive = false;
        }

        if (message.EventType == GameEventType.Collision)
        {
            CollisionCount++;
        }

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
}
