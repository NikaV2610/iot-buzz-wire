namespace Hotwire.Web.Web.Services;
public class CalibrationStateService
{
    public int? ActiveCalibrationId { get; private set; }
    public string? ActiveCalibrationName { get; private set; }

    public void SetActiveCalibration(int calibrationId, string name)
    {
        ActiveCalibrationId = calibrationId;
        ActiveCalibrationName = name;
    }
}
