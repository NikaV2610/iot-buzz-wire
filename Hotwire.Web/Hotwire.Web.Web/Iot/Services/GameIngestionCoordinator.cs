using Hotwire.Web.Web.Data;
using Hotwire.Web.Web.Iot.Contracts;
using Hotwire.Web.Web.Iot.State;
using Hotwire.Web.Web.Models;
using Microsoft.EntityFrameworkCore;

namespace Hotwire.Web.Web.Iot.Services;

public class GameIngestionCoordinator
{
    private readonly IDbContextFactory<GameDbContext> _dbFactory;
    private readonly LiveGameState _state;
    private readonly ILogger<GameIngestionCoordinator> _logger;

    public GameIngestionCoordinator(
        IDbContextFactory<GameDbContext> dbFactory,
        LiveGameState state,
        ILogger<GameIngestionCoordinator> logger)
    {
        _dbFactory = dbFactory;
        _state = state;
        _logger = logger;
    }

    public async Task HandlePositionAsync(PositionMessage msg)
    {
        _state.ApplyPosition(msg);

        if (_state.CurrentRunId is null) return;

        await using var db = await _dbFactory.CreateDbContextAsync();

        db.PositionDatas.Add(new PositionData
        {
            GameRunId = _state.CurrentRunId.Value,
            X = msg.X,
            Y = msg.Y,
            TimeStamp = msg.TimestampUtc == default ? DateTime.UtcNow : msg.TimestampUtc
        });

        await db.SaveChangesAsync();
    }
    public async Task HandleGameEventAsync(GameEventMessage msg)
    {
        _state.ApplyEvent(msg);

        switch (msg.EventType)
        {
            case GameEventType.Start:
                await StartRunAsync(msg);
                break;
            case GameEventType.Collision:
                await IncrementCollisionAsync();
                break;
            case GameEventType.Stop:
                await StopRunAsync(msg);
                break;
        }
    }

    private async Task StartRunAsync(GameEventMessage msg)
    {
        await using var db = await _dbFactory.CreateDbContextAsync();

        var playerId = await ResolvePlayerIdAsync(db, msg);
        var calibrationId = await ResolveCalibrationIdAsync(db, msg);

        var start = msg.TimestampUtc == default ? DateTime.UtcNow : msg.TimestampUtc;

        var run = new GameRun
        {
            PlayerId = playerId,
            CalibrationId = calibrationId,
            StartTime = start,
            Completed = false,
            Collisions = 0
        };

        db.GameRuns.Add(run);
        await db.SaveChangesAsync();

        _state.SetCurrentRun(run.Id, msg.RunExternalId);
        _logger.LogInformation("Run started. RunId={RunId}", run.Id);
    }

    private async Task IncrementCollisionAsync()
    {
        if (_state.CurrentRunId is null) return;

        await using var db = await _dbFactory.CreateDbContextAsync();
        var run = await db.GameRuns.FirstOrDefaultAsync(r => r.Id == _state.CurrentRunId.Value);
        if (run is null) return;

        run.Collisions = (run.Collisions ?? 0) + 1;
        await db.SaveChangesAsync();
    }

    private async Task StopRunAsync(GameEventMessage msg)
    {
        if (_state.CurrentRunId is null) return;

        await using var db = await _dbFactory.CreateDbContextAsync();
        var run = await db.GameRuns.FirstOrDefaultAsync(r => r.Id == _state.CurrentRunId.Value);
        if (run is null) return;

        var end = msg.TimestampUtc == default ? DateTime.UtcNow : msg.TimestampUtc;
        run.EndTime = end;
        run.Duration = Math.Max(0, (end - run.StartTime).TotalSeconds);
        run.Completed = true;

        await db.SaveChangesAsync();

        _state.ClearCurrentRun();
        _logger.LogInformation("Run stopped. RunId={RunId}", run.Id);
    }

    private static async Task<int> ResolvePlayerIdAsync(GameDbContext db, GameEventMessage msg)
    {
        // Minimal fallback: first active player if no RFID resolution yet
        var player = await db.Players.OrderByDescending(p => p.IsActive).ThenBy(p => p.Id).FirstOrDefaultAsync();
        return player?.Id ?? 1;
    }

    private static async Task<int> ResolveCalibrationIdAsync(GameDbContext db, GameEventMessage msg)
    {
        if (!string.IsNullOrWhiteSpace(msg.CalibrationName))
        {
            var found = await db.TrackCalibration.FirstOrDefaultAsync(c => c.Name == msg.CalibrationName);
            if (found is not null) return found.Id;
        }

        var first = await db.TrackCalibration.OrderBy(c => c.Id).FirstOrDefaultAsync();
        return first?.Id ?? 1;
    }
}
