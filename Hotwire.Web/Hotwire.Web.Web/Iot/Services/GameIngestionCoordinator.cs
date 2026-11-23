using Hotwire.Web.Web.Data;
using Hotwire.Web.Web.Iot.Contracts;
using Hotwire.Web.Web.Iot.State;
using Hotwire.Web.Web.Models;
using Hotwire.Web.Web.Services;
using Microsoft.EntityFrameworkCore;

namespace Hotwire.Web.Web.Iot.Services;

public class GameIngestionCoordinator
{
    private readonly IDbContextFactory<GameDbContext> _dbFactory;
    private readonly LiveGameState _state;
    private readonly CalibrationStateService _calibrationState;
    private readonly ILogger<GameIngestionCoordinator> _logger;

    public GameIngestionCoordinator(
        IDbContextFactory<GameDbContext> dbFactory,
        LiveGameState state,
        CalibrationStateService calibrationState,
        ILogger<GameIngestionCoordinator> logger)
    {
        _dbFactory = dbFactory;
        _state = state;
        _calibrationState = calibrationState;
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

    private async Task<int> ResolvePlayerIdAsync(GameDbContext db, GameEventMessage msg)
    {
        // 1) Active player with the scanned RFID card (ignores spaces and upper/lower case)
        if (!string.IsNullOrWhiteSpace(msg.RfidTag))
        {
            var tag = msg.RfidTag.Replace(" ", "").ToUpper();
            var owner = await db.Players.FirstOrDefaultAsync(p =>
                p.IsActive && p.RfidTag != null && p.RfidTag.Replace(" ", "").ToUpper() == tag);
            if (owner is not null) return owner.Id;

            _logger.LogWarning("Unknown RFID tag {Tag}, using fallback player.", msg.RfidTag);
        }

        // 2) Fallback: first active player
        var player = await db.Players.OrderByDescending(p => p.IsActive).ThenBy(p => p.Id).FirstOrDefaultAsync();
        return player?.Id ?? 1;
    }

    private async Task<int> ResolveCalibrationIdAsync(GameDbContext db, GameEventMessage msg)
    {
        if (!string.IsNullOrWhiteSpace(msg.CalibrationName))
        {
            var found = await db.TrackCalibration.FirstOrDefaultAsync(c => c.Name == msg.CalibrationName);
            if (found is not null) return found.Id;
        }

        // Calibration loaded on the Track page (so the best-run replay finds this run)
        if (_calibrationState.ActiveCalibrationId is not null)
        {
            return _calibrationState.ActiveCalibrationId.Value;
        }

        var first = await db.TrackCalibration.OrderBy(c => c.Id).FirstOrDefaultAsync();
        return first?.Id ?? 1;
    }
}
