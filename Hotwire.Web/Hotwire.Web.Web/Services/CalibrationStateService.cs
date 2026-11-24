namespace Hotwire.Web.Web.Services;

public class CalibrationStateService
{
    private readonly object _lock = new();
    private readonly List<CalibrationRecordPoint> _points = [];

    public int? ActiveCalibrationId { get; private set; }
    public string? ActiveCalibrationName { get; private set; }

    // True between the calibration start (event 5) and the stop contact
    public bool IsRecording { get; private set; }

    public void SetActiveCalibration(int calibrationId, string name)
    {
        ActiveCalibrationId = calibrationId;
        ActiveCalibrationName = name;
    }

    public void StartRecording()
    {
        lock (_lock)
        {
            _points.Clear();
            IsRecording = true;
        }
    }

    public void AddPoint(double x, double y, DateTime timestamp)
    {
        lock (_lock)
        {
            if (IsRecording) _points.Add(new CalibrationRecordPoint(x, y, timestamp));
        }
    }

    // Ends the recording and returns all collected camera positions
    public List<CalibrationRecordPoint> StopRecording()
    {
        lock (_lock)
        {
            IsRecording = false;
            var result = new List<CalibrationRecordPoint>(_points);
            _points.Clear();
            return result;
        }
    }
}

public record CalibrationRecordPoint(double X, double Y, DateTime Timestamp);
