using Hotwire.Web.Web.Models;
using Microsoft.EntityFrameworkCore;

namespace Hotwire.Web.Web.Data;

public class GameDbContext : DbContext
{
    public GameDbContext(DbContextOptions<GameDbContext> options) : base(options) { }
    public DbSet<Player> Players => Set<Player>();
    public DbSet<GameRun> GameRuns => Set<GameRun>();
    public DbSet<PositionData> PositionDatas => Set<PositionData>();
    public DbSet<TrackCalibration> TrackCalibration => Set<TrackCalibration>();

    protected override void OnModelCreating(ModelBuilder modelBuilder)
    {
        base.OnModelCreating(modelBuilder);

        modelBuilder.Entity<Player>()
            .HasIndex(p => p.RfidTag)
            .IsUnique();
        modelBuilder.Entity<GameRun>()
            .HasOne<Player>()
            .WithMany()
            .HasForeignKey(r => r.PlayerId)
            .OnDelete(DeleteBehavior.Cascade);

        modelBuilder.Entity<TrackCalibration>()
            .Property(c => c.Name)
            .IsRequired();
        modelBuilder.Entity<GameRun>()
            .HasOne<TrackCalibration>()
            .WithMany()
            .HasForeignKey(r => r.CalibrationId)
            .OnDelete(DeleteBehavior.Cascade);
    }
}

/*
 Im Program.cs wird registrierung durch folgendes:
    builder.Services.AddDbContext<GameDbContext>(options => options.UseSqlite("Data Source=hotwire.db"));
 */