#pragma once

#include <string>

struct InstallPaths
{
    std::string log;
    std::string ini;
    std::string fogData;
    std::string waterData;
    std::string zoneHalo;
    std::string shippedZoneHalo;
};

InstallPaths ClientInstallPaths(const std::string& clientFolder);
