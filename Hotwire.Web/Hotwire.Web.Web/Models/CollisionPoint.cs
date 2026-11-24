namespace Hotwire.Web.Web.Models;

// One wire touch: when it happened and where the camera saw the red tip
public class CollisionPoint
{
    public int Id { get; set; }
    public int GameRunId { get; set; }
    public DateTime TimeStamp { get; set; }

    // Camera position at the touch (null = camera did not see the marker)
    public int? X { get; set; }
    public int? Y { get; set; }
}
