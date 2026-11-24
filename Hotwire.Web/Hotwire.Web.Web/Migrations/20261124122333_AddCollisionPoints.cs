using System;
using Microsoft.EntityFrameworkCore.Migrations;

#nullable disable

namespace Hotwire.Web.Web.Migrations
{
    /// <inheritdoc />
    public partial class AddCollisionPoints : Migration
    {
        /// <inheritdoc />
        protected override void Up(MigrationBuilder migrationBuilder)
        {
            migrationBuilder.CreateTable(
                name: "CollisionPoints",
                columns: table => new
                {
                    Id = table.Column<int>(type: "INTEGER", nullable: false)
                        .Annotation("Sqlite:Autoincrement", true),
                    GameRunId = table.Column<int>(type: "INTEGER", nullable: false),
                    TimeStamp = table.Column<DateTime>(type: "TEXT", nullable: false),
                    X = table.Column<int>(type: "INTEGER", nullable: true),
                    Y = table.Column<int>(type: "INTEGER", nullable: true)
                },
                constraints: table =>
                {
                    table.PrimaryKey("PK_CollisionPoints", x => x.Id);
                    table.ForeignKey(
                        name: "FK_CollisionPoints_GameRuns_GameRunId",
                        column: x => x.GameRunId,
                        principalTable: "GameRuns",
                        principalColumn: "Id",
                        onDelete: ReferentialAction.Cascade);
                });

            migrationBuilder.CreateIndex(
                name: "IX_CollisionPoints_GameRunId",
                table: "CollisionPoints",
                column: "GameRunId");
        }

        /// <inheritdoc />
        protected override void Down(MigrationBuilder migrationBuilder)
        {
            migrationBuilder.DropTable(
                name: "CollisionPoints");
        }
    }
}
