/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "Settings.h"

#include "utilities/FileUtils.h"

using namespace iptvsimple;
using namespace iptvsimple::utilities;

/***************************************************************************
 * PVR settings
 **************************************************************************/
void Settings::ReadFromAddon(const std::string& userPath, const std::string& clientPath)
{
  m_userPath = userPath;
  m_clientPath = clientPath;

  // TVLINK Server
  m_tvlinkIP = kodi::GetSettingString("tvlinkIP", "127.0.0.1");
  m_tvlinkPort = kodi::GetSettingString("tvlinkPort", "2020");
  m_tvlinkUser = kodi::GetSettingString("tvlinkUser");

  // M3U
  if (!m_tvlinkUser.empty())
    m_m3uUrl = "http://" + m_tvlinkIP + ":" + m_tvlinkPort + "/playlist/" + m_tvlinkUser;
  else
    m_m3uUrl = "http://" + m_tvlinkIP + ":" + m_tvlinkPort + "/playlist";
  m_cacheM3U = kodi::GetSettingBoolean("m3uCache", false);
  m_startChannelNumber = kodi::GetSettingInt("startNum", 1);
  m_numberChannelsByM3uOrderOnly = kodi::GetSettingBoolean("numberByOrder", false);
  m_m3uRefreshMode = kodi::GetSettingEnum<RefreshMode>("m3uRefreshMode", RefreshMode::REPEATED_REFRESH);
  m_m3uRefreshIntervalMins = kodi::GetSettingInt("m3uRefreshIntervalMins", 180);
  m_m3uRefreshHour = kodi::GetSettingInt("m3uRefreshHour", 4);

  // EPG
  m_epgUrl = "http://" + m_tvlinkIP + ":" + m_tvlinkPort + "/xmltv";
  m_cacheEPG = kodi::GetSettingBoolean("epgCache", true);
  m_epgTimeShiftHours = kodi::GetSettingFloat("epgTimeShift", 0.0f);
  m_tsOverride = kodi::GetSettingBoolean("epgTSOverride", false);
}

void Settings::ReloadAddonSettings()
{
  ReadFromAddon(m_userPath, m_clientPath);
}

ADDON_STATUS Settings::SetValue(const std::string& settingName, const kodi::CSettingValue& settingValue)
{
  // reset cache and restart addon

  std::string strFile = FileUtils::GetUserDataAddonFilePath(M3U_CACHE_FILENAME);
  if (FileUtils::FileExists(strFile))
    FileUtils::DeleteFile(strFile);

  strFile = FileUtils::GetUserDataAddonFilePath(XMLTV_CACHE_FILENAME);
  if (FileUtils::FileExists(strFile))
    FileUtils::DeleteFile(strFile);

  // TVLINK
  if (settingName == "tvlinkIP")
    return SetStringSetting<ADDON_STATUS>(settingName, settingValue, m_tvlinkIP, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "tvlinkPort")
    return SetStringSetting<ADDON_STATUS>(settingName, settingValue, m_tvlinkPort, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "tvlinkUser")
    return SetStringSetting<ADDON_STATUS>(settingName, settingValue, m_tvlinkUser, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "m3uCache")
    return SetSetting<bool, ADDON_STATUS>(settingName, settingValue, m_cacheM3U, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "startNum")
    return SetSetting<int, ADDON_STATUS>(settingName, settingValue, m_startChannelNumber, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "numberByOrder")
    return SetSetting<bool, ADDON_STATUS>(settingName, settingValue, m_numberChannelsByM3uOrderOnly, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "m3uRefreshMode")
    return SetEnumSetting<RefreshMode, ADDON_STATUS>(settingName, settingValue, m_m3uRefreshMode, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "m3uRefreshIntervalMins")
    return SetSetting<int, ADDON_STATUS>(settingName, settingValue, m_m3uRefreshIntervalMins, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "m3uRefreshHour")
    return SetSetting<int, ADDON_STATUS>(settingName, settingValue, m_m3uRefreshHour, ADDON_STATUS_OK, ADDON_STATUS_OK);
  // EPG
  else if (settingName == "epgCache")
    return SetSetting<bool, ADDON_STATUS>(settingName, settingValue, m_cacheEPG, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "epgTimeShift")
    return SetSetting<float, ADDON_STATUS>(settingName, settingValue, m_epgTimeShiftHours, ADDON_STATUS_OK, ADDON_STATUS_OK);
  else if (settingName == "epgTSOverride")
    return SetSetting<bool, ADDON_STATUS>(settingName, settingValue, m_tsOverride, ADDON_STATUS_OK, ADDON_STATUS_OK);

  return ADDON_STATUS_OK;
}
