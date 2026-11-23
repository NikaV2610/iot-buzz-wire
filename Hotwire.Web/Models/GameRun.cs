namespace Hotwire.Web.Models;

public class GameRun
{
    public int Id { get; set; }

    public int PlayerId { get; set; }

    public DateTime StartTime { get; set; }

    public DateTime? EndTime { get; set; }

    public double Duration { get; set; }

    public int? Collisions { get; set; }

    public bool Completed { get; set; }
}