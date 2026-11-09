using System;

namespace Hotwire.Web.Models;

public class Player
{
    public int Id { get; set; }

    public int RfidTag { get; set; }

    public string FirstName { get; set; }

    public string LastName { get; set; }

    public DateTime CreatedAt { get; set; }
}