var builder = DistributedApplication.CreateBuilder(args);

builder.AddProject<Projects.Hotwire_Web_Web>("webfrontend")
    .WithExternalHttpEndpoints()
    .WithHttpHealthCheck("/health");

builder.Build().Run();
