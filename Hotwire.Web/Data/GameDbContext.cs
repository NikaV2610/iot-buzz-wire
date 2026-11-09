using Hotwire.Web.Models;
using Microsoft.EntityFrameworkCore;

namespace Hotwire.Web.Data;

public class GameDbContext : DbContext
{
    public GameDbContext(DbContextOptions<GameDbContext>) : base(options) { }

    public DbSet<Player> Players => Set<Player>();
    public DbSet<GameRun> GameRuns => Set<GameRuns>();
    public DbSet<positionData> PositionDatas => Set<PositionData>();

    protected override void OnModelCreation(ModelBuilder modelBuilder)
    {
        base.OnModelCreating(modelBuilder);

        modelBuilder.Entity<Players>()
            .HasIndex(p => p.RfidTag)
            .IsUnique();
        modelBuilder.Entity<GameRun>()
            .HasOne<Players>()
            .WithMany()
            .HasForeignKey(r => r.PlayerId)
            .OnDelete(DeleteBehavior.Cascade);
    }
}

/*
 Im Program.cs wird registrierung durch folgendes:
    builder.Services.AddDbContext<GameDbContext>(options => options.UseSqlite("Data Source=hotwire.db"));
 */