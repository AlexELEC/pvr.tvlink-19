/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include "data/Channel.h"
#include "utilities/Logger.h"

#include <string>
#include <type_traits>

#include <kodi/AddonBase.h>

namespace iptvsimple
{
  static const std::string M3U_CACHE_FILENAME = "iptv.m3u.cache";
  static const std::string XMLTV_CACHE_FILENAME = "xmltv.xml.cache";
  static const std::string ADDON_DATA_BASE_DIR = "special://userdata/addon_data/pvr.tvlink";
  static const std::string DEFAULT_GENRE_TEXT_MAP_FILE = ADDON_DATA_BASE_DIR + "/genres/genreTextMappings/genres.xml";
  static const int DEFAULT_UDPXY_MULTICAST_RELAY_PORT = 4022;

  enum class PathType
    : int // same type as addon settings
  {
    LOCAL_PATH = 0,
    REMOTE_PATH
  };

  enum class RefreshMode
    : int // same type as addon settings
  {
    DISABLED = 0,
    REPEATED_REFRESH,
    ONCE_PER_DAY
  };

  enum class EpgLogosMode
    : int // same type as addon settings
  {
    IGNORE_XMLTV = 0,
    PREFER_M3U,
    PREFER_XMLTV
  };

  class Settings
  {
  public:
    /**
     * Singleton getter for the instance
     */
    static Settings& GetInstance()
    {
      static Settings settings;
      return settings;
    }

    void ReadFromAddon(const std::string& userPath, const std::string& clientPath);
    void ReloadAddonSettings();
    ADDON_STATUS SetValue(const std::string& settingName, const kodi::addon::CSettingValue& settingValue);

    const std::string& GetUserPath() const { return m_userPath; }
    const std::string& GetClientPath() const { return m_clientPath; }

    const std::string& GetM3ULocation() const { return m_m3uPathType == PathType::REMOTE_PATH ? m_m3uUrl : m_m3uPath; }
    const PathType& GetM3UPathType() const { return m_m3uPathType; }
    const std::string& GetM3UPath() const { return m_m3uPath; }
    const std::string& GetM3UUrl() const { return m_m3uUrl; }
    bool UseM3UCache() const { return m_m3uPathType == PathType::REMOTE_PATH ? m_cacheM3U : false; }
    int GetStartChannelNumber() const { return m_startChannelNumber; }
    bool NumberChannelsByM3uOrderOnly() const { return m_numberChannelsByM3uOrderOnly; }
    const RefreshMode& GetM3URefreshMode() const { return m_m3uRefreshMode; }
    int GetM3URefreshIntervalMins() const { return m_m3uRefreshIntervalMins; }
    int GetM3URefreshHour() const { return m_m3uRefreshHour; }
    int GetConnectTimeout() const { return m_connectTimeout; }
    bool GetCurlBuffering() const { return m_curlBuff; }

    const std::string& GetEpgLocation() const
    {
      const std::string& epgLocation = m_epgPathType == PathType::REMOTE_PATH ? m_epgUrl : m_epgPath;
      return epgLocation.empty() ? m_tvgUrl : epgLocation;
    }
    const PathType& GetEpgPathType() const { return m_epgPathType; }
    const std::string& GetEpgPath() const { return m_epgPath; }
    const std::string& GetEpgUrl() const { return m_epgUrl; }
    bool UseEPGCache() const { return m_epgPathType == PathType::REMOTE_PATH ? m_cacheEPG : false; }
    float GetEpgTimeshiftHours() const { return m_epgTimeShiftHours; }
    int GetEpgTimeshiftSecs() const { return static_cast<int>(m_epgTimeShiftHours * 60 * 60); }
    bool GetTsOverride() const { return m_tsOverride; }

    const std::string& GetGenresLocation() const { return m_genresPathType == PathType::REMOTE_PATH ? m_genresUrl : m_genresPath; }
    bool UseEpgGenreTextWhenMapping() const { return m_useEpgGenreTextWhenMapping; }
    const PathType& GetGenresPathType() const { return m_genresPathType; }
    const std::string& GetGenresPath() const { return m_genresPath; }
    const std::string& GetGenresUrl() const { return m_genresUrl; }

    const std::string& GetLogoLocation() const { return m_logoPathType == PathType::REMOTE_PATH ? m_logoBaseUrl : m_logoPath; }
    const PathType& GetLogoPathType() const { return m_logoPathType; }
    const std::string& GetLogoPath() const { return m_logoPath; }
    const std::string& GetLogoBaseUrl() const { return m_logoBaseUrl; }
    const EpgLogosMode& GetEpgLogosMode() const { return m_epgLogosMode; }

    bool IsTimeshiftEnabled() const { return m_timeshiftEnabled; }
    bool IsTimeshiftEnabledAll() const { return m_timeshiftEnabledAll; }
    bool IsTimeshiftEnabledHttp() const { return m_timeshiftEnabledHttp; }
    bool IsTimeshiftEnabledUdp() const { return m_timeshiftEnabledUdp; }
    bool AlwaysEnableTimeshiftModeIfMissing() const { return m_timeshiftEnabledCustom; }

    bool IsCatchupEnabled() const { return m_catchupEnabled; }
    const std::string& GetCatchupQueryFormat() const { return m_catchupQueryFormat; }
    int GetCatchupDays() const { return m_catchupDays; }
    time_t GetCatchupDaysInSeconds() const { return static_cast<time_t>(m_catchupDays) * 24 * 60 * 60; }
    const CatchupMode& GetAllChannelsCatchupMode() const { return m_allChannelsCatchupMode; }
    bool CatchupPlayEpgAsLive() const { return m_catchupPlayEpgAsLive; }
    int GetCatchupWatchEpgBeginBufferMins() const { return m_catchupWatchEpgBeginBufferMins; }
    time_t GetCatchupWatchEpgBeginBufferSecs() const { return static_cast<time_t>(m_catchupWatchEpgBeginBufferMins) * 60; }
    int GetCatchupWatchEpgEndBufferMins() const { return m_catchupWatchEpgEndBufferMins; }
    time_t GetCatchupWatchEpgEndBufferSecs() const { return static_cast<time_t>(m_catchupWatchEpgEndBufferMins) * 60; }
    bool CatchupOnlyOnFinishedProgrammes() const { return m_catchupOnlyOnFinishedProgrammes; }

    bool TransformMulticastStreamUrls() const { return m_transformMulticastStreamUrls; }
    const std::string& GetUdpxyHost() const { return m_udpxyHost; }
    int GetUdpxyPort() const { return m_udpxyPort; }
    bool UseFFmpegReconnect() const { return m_useFFmpegReconnect; }
    bool UseInputstreamAdaptiveforHls() const { return m_useInputstreamAdaptiveforHls; }
    const std::string& GetDefaultUserAgent() const { return m_defaultUserAgent; }
    const std::string& GetDefaultInputstream() const { return m_defaultInputstream; }
    const std::string& GetDefaultMimeType() const { return m_defaultMimeType; }

    const std::string& GetTvgUrl() const { return m_tvgUrl; }
    void SetTvgUrl(const std::string& tvgUrl) { m_tvgUrl = tvgUrl; }

  private:
    Settings() = default;

    Settings(Settings const&) = delete;
    void operator=(Settings const&) = delete;

    template<typename T, typename V>
    V SetSetting(const std::string& settingName, const kodi::addon::CSettingValue& settingValue, T& currentValue, V returnValueIfChanged, V defaultReturnValue)
    {
      T newValue;
      if (std::is_same<T, float>::value)
        newValue = static_cast<T>(settingValue.GetFloat());
      else if (std::is_same<T, bool>::value)
        newValue = static_cast<T>(settingValue.GetBoolean());
      else if (std::is_same<T, unsigned int>::value)
        newValue = static_cast<T>(settingValue.GetUInt());
      else
        newValue = static_cast<T>(settingValue.GetInt());

      if (newValue != currentValue)
      {
        std::string formatString = "%s - Changed Setting '%s' from %d to %d";
        if (std::is_same<T, float>::value)
          formatString = "%s - Changed Setting '%s' from %f to %f";
        utilities::Logger::Log(utilities::LogLevel::LEVEL_INFO, formatString.c_str(), __FUNCTION__, settingName.c_str(), currentValue, newValue);
        currentValue = newValue;
        return returnValueIfChanged;
      }

      return defaultReturnValue;
    }

    template<typename T, typename V>
    V SetEnumSetting(const std::string& settingName, const kodi::addon::CSettingValue& settingValue, T& currentValue, V returnValueIfChanged, V defaultReturnValue)
    {
      T newValue = settingValue.GetEnum<T>();
      if (newValue != currentValue)
      {
        utilities::Logger::Log(utilities::LogLevel::LEVEL_INFO, "%s - Changed Setting '%s' from %d to %d", __FUNCTION__, settingName.c_str(), currentValue, newValue);
        currentValue = newValue;
        return returnValueIfChanged;
      }

      return defaultReturnValue;
    }

    template<typename V>
    V SetStringSetting(const std::string& settingName, const kodi::addon::CSettingValue& settingValue, std::string& currentValue, V returnValueIfChanged, V defaultReturnValue)
    {
      const std::string strSettingValue = settingValue.GetString();

      if (strSettingValue != currentValue)
      {
        utilities::Logger::Log(utilities::LogLevel::LEVEL_INFO, "%s - Changed Setting '%s' from '%s' to '%s'", __FUNCTION__, settingName.c_str(), currentValue.c_str(), strSettingValue.c_str());
        currentValue = strSettingValue;
        return returnValueIfChanged;
      }

      return defaultReturnValue;
    }

    std::string m_userPath;
    std::string m_clientPath;

    // TVLINK Server
    std::string m_tvlinkIP;
    std::string m_tvlinkPort;
    std::string m_tvlinkUser;
    std::string m_tvlinkToken;
    std::string m_tvlinkList;
    int m_connectTimeout = 10;
    bool m_curlBuff = false;
    bool m_useFFmpeg = false;

    // M3U
    PathType m_m3uPathType = PathType::REMOTE_PATH;
    std::string m_m3uPath;
    std::string m_m3uUrl;
    bool m_cacheM3U = false;
    int m_startChannelNumber = 1;
    bool m_numberChannelsByM3uOrderOnly = false;
    RefreshMode m_m3uRefreshMode = RefreshMode::REPEATED_REFRESH;
    int m_m3uRefreshIntervalMins = 180;
    int m_m3uRefreshHour = 4;

    // EPG
    PathType m_epgPathType = PathType::REMOTE_PATH;
    std::string m_epgPath;
    std::string m_epgUrl;
    bool m_cacheEPG = true;
    float m_epgTimeShiftHours = 0;
    bool m_tsOverride = false;

    // Genres
    bool m_useEpgGenreTextWhenMapping = false;
    PathType m_genresPathType = PathType::LOCAL_PATH;
    std::string m_genresPath;
    std::string m_genresUrl;

    // Channel Logos
    PathType m_logoPathType = PathType::REMOTE_PATH;
    std::string m_logoPath;
    std::string m_logoBaseUrl;
    EpgLogosMode m_epgLogosMode = EpgLogosMode::PREFER_M3U;

    // Timeshift
    bool m_timeshiftEnabled = true;
    bool m_timeshiftEnabledAll = true;
    bool m_timeshiftEnabledHttp = false;
    bool m_timeshiftEnabledUdp = false;
    bool m_timeshiftEnabledCustom = false;

    // Catchup
    bool m_catchupEnabled = true;
    std::string m_catchupQueryFormat;
    int m_catchupDays = 8;
    CatchupMode m_allChannelsCatchupMode = CatchupMode::SHIFT;
    bool m_catchupPlayEpgAsLive = true;
    int m_catchupWatchEpgBeginBufferMins = 5;
    int m_catchupWatchEpgEndBufferMins = 15;
    bool m_catchupOnlyOnFinishedProgrammes = false;

    // Advanced
    bool m_transformMulticastStreamUrls = false;
    std::string m_udpxyHost;
    int m_udpxyPort = DEFAULT_UDPXY_MULTICAST_RELAY_PORT;
    bool m_useFFmpegReconnect = true;
    bool m_useInputstreamAdaptiveforHls = false;
    std::string m_defaultUserAgent;
    std::string m_defaultInputstream;
    std::string m_defaultMimeType;

    std::string m_tvgUrl;
  };
} //namespace iptvsimple
