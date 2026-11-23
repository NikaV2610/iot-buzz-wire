using System;
using Microsoft.EntityFrameworkCore.Migrations;

#nullable disable

namespace Hotwire.Web.Web.Migrations
{
    /// <inheritdoc />
    public partial class AddTrackCalibrationLink : Migration
    {
        /// <inheritdoc />
        protected override void Up(MigrationBuilder migrationBuilder)
        {
            migrationBuilder.RenameColumn(
                name: "DeaktivatedAT",
                table: "Players",
                newName: "DeaktivatedAt");

            migrationBuilder.AddColumn<int>(
                name: "CalibrationId",
                table: "GameRuns",
                type: "INTEGER",
                nullable: false,
                defaultValue: 1);

            migrationBuilder.CreateTable(
                name: "TrackCalibration",
                columns: table => new
                {
                    Id = table.Column<int>(type: "INTEGER", nullable: false)
                        .Annotation("Sqlite:Autoincrement", true),
                    Name = table.Column<string>(type: "TEXT", nullable: false),
                    CreatedAt = table.Column<DateTime>(type: "TEXT", nullable: false)
                },
                constraints: table =>
                {
                    table.PrimaryKey("PK_TrackCalibration", x => x.Id);
                });

            migrationBuilder.CreateIndex(
                name: "IX_GameRuns_CalibrationId",
                table: "GameRuns",
                column: "CalibrationId");

            migrationBuilder.InsertData(
                table: "TrackCalibration",
                columns: new[] { "Id", "Name", "CreatedAt" },
                values: new object[] { 1, "Default Calibration", new DateTime(2026, 11, 20, 0, 0, 0, DateTimeKind.Utc) });

            migrationBuilder.Sql("UPDATE GameRuns SET CalibrationId = 1 WHERE CalibrationId = 0;");

            migrationBuilder.AddForeignKey(
                name: "FK_GameRuns_TrackCalibration_CalibrationId",
                table: "GameRuns",
                column: "CalibrationId",
                principalTable: "TrackCalibration",
                principalColumn: "Id",
                onDelete: ReferentialAction.Cascade);
        }

        /// <inheritdoc />
        protected override void Down(MigrationBuilder migrationBuilder)
        {
            migrationBuilder.DropForeignKey(
                name: "FK_GameRuns_TrackCalibration_CalibrationId",
                table: "GameRuns");

            migrationBuilder.DropTable(
                name: "TrackCalibration");

            migrationBuilder.DropIndex(
                name: "IX_GameRuns_CalibrationId",
                table: "GameRuns");

            migrationBuilder.DropColumn(
                name: "CalibrationId",
                table: "GameRuns");

            migrationBuilder.RenameColumn(
                name: "DeaktivatedAt",
                table: "Players",
                newName: "DeaktivatedAT");
        }
    }
}
