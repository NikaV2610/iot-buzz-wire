namespace Hotwire.Web.Web.Models;

public class Player
{
    public int Id { get; set; }
    public string? RfidTag { get; set; }
    public bool IsActive { get; set; } = true;
    public string FirstName { get; set; }
    public string LastName { get; set; }
    public DateTime CreatedAt { get; set; }

    public DateTime? DeactivatedAt { get; set; }

}