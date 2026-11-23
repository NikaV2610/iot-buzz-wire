using Microsoft.EntityFrameworkCore.Migrations;

#nullable disable

namespace Hotwire.Web.Web.Migrations
{
    /// <inheritdoc />
    public partial class RenamePlayerDeactivatedAt : Migration
    {
        /// <inheritdoc />
        protected override void Up(MigrationBuilder migrationBuilder)
        {
            migrationBuilder.RenameColumn(
                name: "DeaktivatedAt",
                table: "Players",
                newName: "DeactivatedAt");
        }

        /// <inheritdoc />
        protected override void Down(MigrationBuilder migrationBuilder)
        {
            migrationBuilder.RenameColumn(
                name: "DeactivatedAt",
                table: "Players",
                newName: "DeaktivatedAt");
        }
    }
}
