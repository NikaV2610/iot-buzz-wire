using Hotwire.Web.Web.Components;
using Hotwire.Web.Web.Data;
using Hotwire.Web.Web.Iot.Configuration;
using Hotwire.Web.Web.Iot.Services;
using Hotwire.Web.Web.Iot.State;
using Hotwire.Web.Web.Services;
using Microsoft.EntityFrameworkCore;

var builder = WebApplication.CreateBuilder(args);

// Add service defaults & Aspire client integrations.
builder.AddServiceDefaults();

// Add services to the container.
builder.Services.AddRazorComponents()
    .AddInteractiveServerComponents();

builder.Services.AddOutputCache();

builder.Services.AddDbContextFactory<GameDbContext>(options =>
    options.UseSqlite("Data Source=hotwire.db"));

builder.Services.Configure<MqttOptions>(builder.Configuration.GetSection("Mqtt"));
builder.Services.AddSingleton<LiveGameState>();
builder.Services.AddHostedService<MqttIngestionService>();

builder.Services.AddSingleton<CalibrationStateService>();

builder.Services.AddScoped<GameIngestionCoordinator>();

var app = builder.Build();

if (!app.Environment.IsDevelopment())
{
    app.UseExceptionHandler("/Error", createScopeForErrors: true);
    // The default HSTS value is 30 days. You may want to change this for production scenarios, see https://aka.ms/aspnetcore-hsts.
    app.UseHsts();
}

app.UseHttpsRedirection();

app.UseAntiforgery();

app.UseOutputCache();

app.MapStaticAssets();

app.MapRazorComponents<App>()
    .AddInteractiveServerRenderMode();

app.MapDefaultEndpoints();

app.Run();
