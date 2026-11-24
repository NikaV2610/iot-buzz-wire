namespace Hotwire.Web.Web.Services;

// Score of a run: fast AND clean wins.
//   points = 1000 - 10 per second - 10 per error   (never below 0)
// One error costs as much as 1 second (tested: players make 20-30 errors per run).
// The score is calculated, not stored: change the numbers here
// and the scoreboard uses the new rule for all runs.
public static class ScoreCalculator
{
    public const int StartPoints = 1000;
    public const int PointsPerSecond = 10;
    public const int PointsPerError = 10;

    public static int Points(double seconds, int errors)
    {
        var points = StartPoints - PointsPerSecond * seconds - PointsPerError * errors;
        return Math.Max(0, (int)Math.Round(points));
    }

    // Short text for the pages, e.g. "1000 − 10 per second − 10 per error"
    public static string Rule => $"{StartPoints} − {PointsPerSecond} per second − {PointsPerError} per error";
}
