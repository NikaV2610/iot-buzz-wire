using Hotwire.Web.Web.Iot.Contracts;

namespace Hotwire.Web.Web.Iot.State;

public sealed class LiveGameState
{
    public PositionMessage? LastPosition { get; private set; }
    public GameEventMessage? LastEvent { get; private set; }
    public int CollisionCount { get; private set; }
    public bool IsRunActive { get; private set; }

    public event Action? Changed;

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
}
