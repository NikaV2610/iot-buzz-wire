using Hotwire.Web.Web.Data;
using Hotwire.Web.Web.Iot.Contracts;
using Hotwire.Web.Web.Iot.State;
using Hotwire.Web.Web.Models;
using Hotwire.Web.Web.Services;
using Microsoft.EntityFrameworkCore;
using System.Text.Json;

namespace Hotwire.Web.Web.Iot.Services;

public class GameIngestionCoordinator
{
    private readonly IDbContextFactory<GameDbContext> _dbFactory;
    private readonly LiveGameState _state;
    private readonly CalibrationStateService _calibrationState;
    private readonly IWebHostEnvironment _env;
    private readonly ILogger<GameIngestionCoordinator> _logger;

    public GameIngestionCoordinator(
        IDbContextFactory<GameDbContext> dbFactory,
        LiveGameState state,
        CalibrationStateService calibrationState,
        IWebHostEnvironment env,
        ILogger<GameIngestionCoordinator> logger)
    {
        _dbFactory = dbFactory;
        _state = state;
        _calibrationState = calibrationState;
        _env = env;
        _logger = logger;
    }

    public async Task HandlePositionAsync(PositionMessage msg)
    {
        _state.ApplyPosition(msg);

        // Calibration run: collect the path instead of storing it for a game run
        if (_calibrationState.IsRecording)
        {
            _calibrationState.AddPoint(msg.X, msg.Y, msg.TimestampUtc == default ? DateTime.UtcNow : msg.TimestampUtc);
            return;
        }

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
    // RFID topic: remember the card, the next runs belong to this player
    public async Task HandleRfidAsync(RfidMessage msg)
    {
        if (string.IsNullOrWhiteSpace(msg.Number)) return;

        var number = msg.Number.Replace(" ", "").ToUpper();

        // Look up the name only for the live display
        await using var db = await _dbFactory.CreateDbContextAsync();
        var player = await db.Players.FirstOrDefaultAsync(p =>
            p.IsActive && p.RfidTag != null && p.RfidTag.Replace(" ", "").ToUpper() == number);

        _state.SetRfid(number, player is null ? null : $"{player.FirstName} {player.LastName}");
        _logger.LogInformation("Card scanned: {Number} ({Player})", number, _state.CurrentPlayerName ?? "unknown");
    }

    public async Task HandleGameEventAsync(GameEventMessage msg)
    {
        _state.ApplyEvent(msg);

        // Red button ("newPlayer"): forget the old card, wait for the next one
        if (msg.EventType == GameEventType.PlayerRegistration)
        {
            _state.SetRfid(null, null);
            _logger.LogInformation("New player: waiting for a card.");
            return;
        }

        // Calibration: event 5 starts recording, the next Stop saves the track
        if (msg.EventType == GameEventType.CalibrationSelected)
        {
            _calibrationState.StartRecording();
            _state.NotifyChanged();
            _logger.LogInformation("Calibration recording started.");
            return;
        }

        if (msg.EventType == GameEventType.Stop && _calibrationState.IsRecording)
        {
            await SaveCalibrationAsync();
            _state.NotifyChanged();
            return;
        }

        switch (msg.EventType)
        {
            case GameEventType.Start:
                await StartRunAsync(msg);
                break;
            case GameEventType.Collision:
                await IncrementCollisionAsync(msg);
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

    private async Task IncrementCollisionAsync(GameEventMessage msg)
    {
        if (_state.CurrentRunId is null) return;

        await using var db = await _dbFactory.CreateDbContextAsync();
        var run = await db.GameRuns.FirstOrDefaultAsync(r => r.Id == _state.CurrentRunId.Value);
        if (run is null) return;

        run.Collisions = (run.Collisions ?? 0) + 1;

        // Remember where the wire was touched: last camera position (if it is fresh)
        var pos = _state.LastPosition;
        var posIsFresh = pos is not null && (msg.TimestampUtc - pos.TimestampUtc).Duration() < TimeSpan.FromSeconds(1);
        db.CollisionPoints.Add(new CollisionPoint
        {
            GameRunId = run.Id,
            TimeStamp = msg.TimestampUtc,
            X = posIsFresh ? pos!.X : null,
            Y = posIsFresh ? pos!.Y : null
        });

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

    // Saves the recorded camera path as new track (calibration.json + database)
    private async Task SaveCalibrationAsync()
    {
        var raw = _calibrationState.StopRecording();
        if (raw.Count < 2)
        {
            _logger.LogWarning("Calibration not saved: only {Count} positions received. Is the camera running?", raw.Count);
            return;
        }

        // Remove jumps and smooth automatically before saving
        var cleaned = TrackSmoothing.Clean(raw.Select(p => (p.X, p.Y)).ToList());
        var points = raw.Select((p, i) => new { X = cleaned[i].X, Y = cleaned[i].Y, p.Timestamp }).ToList();

        var name = $"Track {DateTime.Now:yyyy-MM-dd HH:mm:ss}";

        // Same format as before, the Track page reads this file
        var data = new
        {
            trackName = name,
            points = points.Select(p => new { x = p.X, y = p.Y, timestamp = p.Timestamp })
        };
        var filePath = Path.Combine(_env.WebRootPath, "data", "calibration.json");
        await File.WriteAllTextAsync(filePath, JsonSerializer.Serialize(data, new JsonSerializerOptions { WriteIndented = true }));

        await using var db = await _dbFactory.CreateDbContextAsync();
        var calibration = new TrackCalibration { Name = name, CreatedAt = DateTime.UtcNow };
        db.TrackCalibration.Add(calibration);
        await db.SaveChangesAsync();

        _calibrationState.SetActiveCalibration(calibration.Id, calibration.Name);
        _logger.LogInformation("Calibration saved: {Name} with {Count} points.", name, points.Count);
    }

    private async Task<int> ResolvePlayerIdAsync(GameDbContext db, GameEventMessage msg)
    {
        // 1) Active player with the scanned RFID card (ignores spaces and upper/lower case).
        //    The card comes from the RFID topic; rfidTag in the game event still works too.
        var rfid = !string.IsNullOrWhiteSpace(msg.RfidTag) ? msg.RfidTag : _state.CurrentRfid;
        if (!string.IsNullOrWhiteSpace(rfid))
        {
            var tag = rfid.Replace(" ", "").ToUpper();
            var owner = await db.Players.FirstOrDefaultAsync(p =>
                p.IsActive && p.RfidTag != null && p.RfidTag.Replace(" ", "").ToUpper() == tag);
            if (owner is not null) return owner.Id;

            _logger.LogWarning("Unknown RFID tag {Tag}, using fallback player.", rfid);
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

        // After a restart nothing is active yet: use the newest calibration
        var newest = await db.TrackCalibration.OrderByDescending(c => c.Id).FirstOrDefaultAsync();
        if (newest is null) return 1;

        _calibrationState.SetActiveCalibration(newest.Id, newest.Name);
        return newest.Id;
    }
}
