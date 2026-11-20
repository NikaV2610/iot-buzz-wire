namespace Hotwire.Web.Web.Models;
public class TrackCalibration
{
    public int Id { get; set; }
    public string Name { get; set; } = "";
    public DateTime CreatedAt { get; set; } = DateTime.UtcNow;
}
