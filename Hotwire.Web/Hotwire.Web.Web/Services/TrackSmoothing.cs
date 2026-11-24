namespace Hotwire.Web.Web.Services;

// Cleans camera paths before they are shown or saved:
// 1) jump filter: big jumps (e.g. skin detected) are replaced by the last good point
// 2) median filter (7 points) removes single wrong points (outliers)
// 3) moving average (5 points) smooths the jitter
// The number of points stays the same, so replay timing still fits.
public static class TrackSmoothing
{
    private const double MaxJump = 120;   // pixels between two messages (10 per second)
    private const int JumpConfirm = 5;    // a new position must stay this long to be accepted

    public static List<(double X, double Y)> Clean(IReadOnlyList<(double X, double Y)> points)
    {
        var withoutJumps = RemoveJumps(points);
        var withoutOutliers = Filter(withoutJumps, 3, Median);
        return Filter(withoutOutliers, 2, Average);
    }

    private static List<(double X, double Y)> RemoveJumps(IReadOnlyList<(double X, double Y)> points)
    {
        var result = new List<(double X, double Y)>(points.Count);
        if (points.Count == 0) return result;

        var lastGood = points[0];
        int farCount = 0;

        foreach (var p in points)
        {
            var jump = Math.Sqrt(Math.Pow(p.X - lastGood.X, 2) + Math.Pow(p.Y - lastGood.Y, 2));

            if (jump <= MaxJump || ++farCount >= JumpConfirm)
            {
                lastGood = p;   // normal step, or the new position stayed long enough
                farCount = 0;
            }

            result.Add(lastGood);
        }

        return result;
    }

    private static List<(double X, double Y)> Filter(
        IReadOnlyList<(double X, double Y)> points, int radius, Func<List<double>, double> combine)
    {
        var result = new List<(double X, double Y)>(points.Count);

        for (int i = 0; i < points.Count; i++)
        {
            // Window around point i, smaller at the start and end
            int from = Math.Max(0, i - radius);
            int to = Math.Min(points.Count - 1, i + radius);

            var xs = new List<double>();
            var ys = new List<double>();
            for (int j = from; j <= to; j++)
            {
                xs.Add(points[j].X);
                ys.Add(points[j].Y);
            }

            result.Add((combine(xs), combine(ys)));
        }

        return result;
    }

    private static double Median(List<double> values)
    {
        values.Sort();
        int mid = values.Count / 2;
        return values.Count % 2 == 1 ? values[mid] : (values[mid - 1] + values[mid]) / 2.0;
    }

    private static double Average(List<double> values) => values.Average();
}
