/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "PVRIptvData.h"

#include "iptvsimple/Settings.h"
#include "iptvsimple/utilities/Logger.h"
#include "iptvsimple/utilities/TimeUtils.h"
#include "iptvsimple/utilities/WebUtils.h"

#include <ctime>
#include <chrono>

using namespace iptvsimple;
using namespace iptvsimple::data;
using namespace iptvsimple::utilities;

PVRIptvData::PVRIptvData()
{
  m_channels.Clear();
  m_channelGroups.Clear();
  m_epg.Clear();
}

ADDON_STATUS PVRIptvData::Create()
{
  std::lock_guard<std::mutex> lock(m_mutex);

  /* Configure the logger */
  Logger::GetInstance().SetImplementation([](LogLevel level, const char* message)
  {
    /* Convert the log level */
    ADDON_LOG addonLevel;

    switch (level)
    {
      case LogLevel::LEVEL_FATAL:
        addonLevel = ADDON_LOG_FATAL;
        break;
      case LogLevel::LEVEL_ERROR:
        addonLevel = ADDON_LOG_ERROR;
        break;
      case LogLevel::LEVEL_WARNING:
        addonLevel = ADDON_LOG_WARNING;
        break;
      case LogLevel::LEVEL_INFO:
        addonLevel = ADDON_LOG_INFO;
        break;
      default:
        addonLevel = ADDON_LOG_DEBUG;
    }

    kodi::Log(addonLevel, "%s", message);
  });

  Logger::GetInstance().SetPrefix("pvr.tvlink");

  Logger::Log(LogLevel::LEVEL_INFO, "%s - Creating the PVR TVLINK add-on", __FUNCTION__);

  Settings::GetInstance().ReadFromAddon(kodi::addon::GetUserPath(), kodi::addon::GetAddonPath());

  m_channels.Init();
  m_channelGroups.Init();
  m_playlistLoader.Init();
  if (!m_playlistLoader.LoadPlayList())
  {
    m_channels.ChannelsLoadFailed();
    m_channelGroups.ChannelGroupsLoadFailed();
  }
  m_epg.Init(EpgMaxPastDays(), EpgMaxFutureDays());

  kodi::Log(ADDON_LOG_INFO, "%s Starting separate client update thread...", __FUNCTION__);

  m_running = true;
  m_thread = std::thread([&] { Process(); });
  iCurl_timeout = Settings::GetInstance().GetTvlinkTimeout(); // CURL connection timeout

  return ADDON_STATUS_OK;
}

PVR_ERROR PVRIptvData::GetCapabilities(kodi::addon::PVRCapabilities& capabilities)
{
  capabilities.SetSupportsEPG(true);
  capabilities.SetSupportsTV(true);
  capabilities.SetSupportsRadio(true);
  capabilities.SetSupportsChannelGroups(true);
  capabilities.SetSupportsRecordings(false);
  capabilities.SetSupportsRecordingsRename(false);
  capabilities.SetSupportsRecordingsLifetimeChange(false);
  capabilities.SetSupportsDescrambleInfo(false);
  capabilities.SetHandlesInputStream(true);
  capabilities.SetHandlesDemuxing(false);

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR PVRIptvData::GetBackendName(std::string& name)
{
  name = "TVLINK PVR Add-on";
  return PVR_ERROR_NO_ERROR;
}
PVR_ERROR PVRIptvData::GetBackendVersion(std::string& version)
{
  version = STR(IPTV_VERSION);
  return PVR_ERROR_NO_ERROR;
}
PVR_ERROR PVRIptvData::GetConnectionString(std::string& connection)
{
  connection = "connected";
  return PVR_ERROR_NO_ERROR;
}

void PVRIptvData::Process()
{
  unsigned int refreshTimer = 0;
  time_t lastRefreshTimeSeconds = std::time(nullptr);
  int lastRefreshHour = Settings::GetInstance().GetM3URefreshHour(); //ignore if we start during same hour

  while (m_running)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(PROCESS_LOOP_WAIT_SECS * 1000));

    time_t currentRefreshTimeSeconds = std::time(nullptr);
    std::tm timeInfo = SafeLocaltime(currentRefreshTimeSeconds);
    refreshTimer += static_cast<unsigned int>(currentRefreshTimeSeconds - lastRefreshTimeSeconds);
    lastRefreshTimeSeconds = currentRefreshTimeSeconds;

    if (Settings::GetInstance().GetM3URefreshMode() == RefreshMode::REPEATED_REFRESH &&
        refreshTimer >= (Settings::GetInstance().GetM3URefreshIntervalMins() * 60))
      m_reloadChannelsGroupsAndEPG = true;

    if (Settings::GetInstance().GetM3URefreshMode() == RefreshMode::ONCE_PER_DAY &&
        lastRefreshHour != timeInfo.tm_hour && timeInfo.tm_hour == Settings::GetInstance().GetM3URefreshHour())
      m_reloadChannelsGroupsAndEPG = true;

    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_running && m_reloadChannelsGroupsAndEPG)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));

      Settings::GetInstance().ReloadAddonSettings();
      m_playlistLoader.ReloadPlayList();
      m_epg.ReloadEPG();

      m_reloadChannelsGroupsAndEPG = false;
      refreshTimer = 0;
    }
    lastRefreshHour = timeInfo.tm_hour;
  }
}

PVRIptvData::~PVRIptvData()
{
  Logger::Log(LEVEL_DEBUG, "%s Stopping update thread...", __FUNCTION__);
  m_running = false;
  if (m_thread.joinable())
    m_thread.join();

  std::lock_guard<std::mutex> lock(m_mutex);
  m_channels.Clear();
  m_channelGroups.Clear();
  m_epg.Clear();
}

PVR_ERROR PVRIptvData::GetChannelsAmount(int& amount)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  amount = m_channels.GetChannelsAmount();
  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR PVRIptvData::GetChannels(bool radio, kodi::addon::PVRChannelsResultSet& results)
{
  std::lock_guard<std::mutex> lock(m_mutex);

  return m_channels.GetChannels(results, radio);
}

bool PVRIptvData::GetChannel(const kodi::addon::PVRChannel& channel, Channel& myChannel)
{
  std::lock_guard<std::mutex> lock(m_mutex);

  return m_channels.GetChannel(channel, myChannel);
}

bool PVRIptvData::GetChannel(unsigned int uniqueChannelId, iptvsimple::data::Channel& myChannel)
{
  std::lock_guard<std::mutex> lock(m_mutex);

  return m_channels.GetChannel(uniqueChannelId, myChannel);
}

PVR_ERROR PVRIptvData::GetChannelGroupsAmount(int& amount)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  amount = m_channelGroups.GetChannelGroupsAmount();
  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR PVRIptvData::GetChannelGroups(bool radio, kodi::addon::PVRChannelGroupsResultSet& results)
{
  std::lock_guard<std::mutex> lock(m_mutex);

  return m_channelGroups.GetChannelGroups(results, radio);
}

PVR_ERROR PVRIptvData::GetChannelGroupMembers(const kodi::addon::PVRChannelGroup& group, kodi::addon::PVRChannelGroupMembersResultSet& results)
{
  std::lock_guard<std::mutex> lock(m_mutex);

  return m_channelGroups.GetChannelGroupMembers(group, results);
}

PVR_ERROR PVRIptvData::GetEPGForChannel(int channelUid, time_t start, time_t end, kodi::addon::PVREPGTagsResultSet& results)
{
  std::lock_guard<std::mutex> lock(m_mutex);

  return m_epg.GetEPGForChannel(channelUid, start, end, results);
}

PVR_ERROR PVRIptvData::GetEPGTagStreamProperties(const kodi::addon::PVREPGTag& tag, std::vector<kodi::addon::PVRStreamProperty>& properties)
{
  Logger::Log(LEVEL_DEBUG, "%s - Tag startTime: %ld \tendTime: %ld", __FUNCTION__, tag.GetStartTime(), tag.GetEndTime());

  if (GetChannel(static_cast<int>(tag.GetUniqueChannelId()), m_currentChannel))
  {
    Logger::Log(LEVEL_DEBUG, "%s - GetPlayEpgAsLive is %s", __FUNCTION__, Settings::GetInstance().CatchupPlayEpgAsLive() ? "enabled" : "disabled");

    std::map<std::string, std::string> catchupProperties;
    m_catchupController.ProcessEPGTagForTimeshiftedPlayback(tag, m_currentChannel, catchupProperties);
    std::string orgUrl = m_currentChannel.GetStreamURL();

    // shift catchup url
    m_currentChannel.GenerateShiftCatchupSource(orgUrl);
    const std::string catchupShiftUrl = m_catchupController.GetCatchupUrl(m_currentChannel);
      
    StreamUtils::SetAllStreamProperties(properties, m_currentChannel, catchupShiftUrl, false, catchupProperties);

    Logger::Log(LEVEL_INFO, "%s - EPG Catchup URL: %s", __FUNCTION__, WebUtils::RedactUrl(catchupShiftUrl).c_str());
    return PVR_ERROR_NO_ERROR;
  }

  return PVR_ERROR_FAILED;
}

PVR_ERROR PVRIptvData::IsEPGTagPlayable(const kodi::addon::PVREPGTag& tag, bool& bIsPlayable)
{
  if (!Settings::GetInstance().IsCatchupEnabled())
    return PVR_ERROR_NOT_IMPLEMENTED;

  const time_t now = std::time(nullptr);
  Channel channel;

  // Get the channel and set the current tag on it if found
  bIsPlayable = GetChannel(static_cast<int>(tag.GetUniqueChannelId()), channel) &&
                Settings::GetInstance().IsCatchupEnabled() && channel.IsCatchupSupported();

  if (channel.IgnoreCatchupDays())
  {
    // If we ignore catchup days then any tag can be played but only if it has a catchup ID
    bool hasCatchupId = false;
    EpgEntry* epgEntry = m_catchupController.GetEPGEntry(channel, tag.GetStartTime());
    if (epgEntry)
      hasCatchupId = !epgEntry->GetCatchupId().empty();

    bIsPlayable = bIsPlayable && hasCatchupId;
  }
  else
  {
    bIsPlayable = bIsPlayable &&
                  tag.GetStartTime() < now &&
                  tag.GetStartTime() >= (now - static_cast<time_t>(channel.GetCatchupDaysInSeconds())) &&
                  (!Settings::GetInstance().CatchupOnlyOnFinishedProgrammes() || tag.GetEndTime() < now);
  }

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR PVRIptvData::SetEPGMaxPastDays(int epgMaxPastDays)
{
  m_epg.SetEPGMaxPastDays(epgMaxPastDays);
  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR PVRIptvData::SetEPGMaxFutureDays(int epgMaxFutureDays)
{
  m_epg.SetEPGMaxFutureDays(epgMaxFutureDays);
  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR PVRIptvData::GetSignalStatus(int channelUid, kodi::addon::PVRSignalStatus& signalStatus)
{
  signalStatus.SetAdapterName("TVLINK Adapter 1");
  signalStatus.SetAdapterStatus("OK");

  return PVR_ERROR_NO_ERROR;
}

ADDON_STATUS PVRIptvData::SetSetting(const std::string& settingName, const kodi::addon::CSettingValue& settingValue)
{
  std::lock_guard<std::mutex> lock(m_mutex);

  // When a number of settings change set this on the first one so it can be picked up
  // in the process call for a reload of channels, groups and EPG.
  if (!m_reloadChannelsGroupsAndEPG)
    m_reloadChannelsGroupsAndEPG = true;

  return Settings::GetInstance().SetValue(settingName, settingValue);
}

// live stream functions

bool PVRIptvData::OpenLiveStream(const kodi::addon::PVRChannel& channel)
{
  if (GetChannel(channel, m_currentChannel))
  {
    CloseLiveStream();
    std::string url = m_currentChannel.GetStreamURL();
    std::string name = m_currentChannel.GetChannelName();
    Logger::Log(LogLevel::LEVEL_INFO, "%s - [%s]: %s", __FUNCTION__, name.c_str(), WebUtils::RedactUrl(url).c_str());

    m_streamHandle.CURLCreate(url.c_str());
    if (iCurl_timeout > 0)
      m_streamHandle.CURLAddOption(ADDON_CURL_OPTION_PROTOCOL, "connection-timeout", std::to_string(iCurl_timeout).c_str());
    // ADDON_READ_TRUNCATED | ADDON_READ_CHUNKED | ADDON_READ_NO_CACHE
    m_streamHandle.CURLOpen(ADDON_READ_TRUNCATED | ADDON_READ_CHUNKED | ADDON_READ_NO_CACHE);

    return m_streamHandle.IsOpen();
  }
  return false;
}

void PVRIptvData::CloseLiveStream(void)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  strLastURL = "";
  iRestart_cnt = 0;
  if (m_streamHandle.IsOpen())
  {
    std::string url = m_currentChannel.GetStreamURL();
    std::string name = m_currentChannel.GetChannelName();
    Logger::Log(LogLevel::LEVEL_INFO, "%s - [%s] Live URL: %s", __FUNCTION__, name.c_str(), WebUtils::RedactUrl(url).c_str());
    m_streamHandle.Close();
  }

}

int PVRIptvData::ReadLiveStream(unsigned char *pBuffer, unsigned int iBufferSize)
{
  unsigned int bytesRead = m_streamHandle.Read(pBuffer, iBufferSize);
  if (bytesRead < 50)
  {
    std::string url = m_currentChannel.GetStreamURL();
    std::string name = m_currentChannel.GetChannelName();
    Logger::Log(LogLevel::LEVEL_INFO, "%s - [%s] requested bytes [%d] received bytes [%d]", __FUNCTION__, name.c_str(), iBufferSize, bytesRead);
    Logger::Log(LogLevel::LEVEL_INFO, "%s - last URL [%s] | current URL [%s]", __FUNCTION__, WebUtils::RedactUrl(strLastURL).c_str(), WebUtils::RedactUrl(url).c_str());
    if (strLastURL == url)
    {
      iRestart_cnt++;
    }
    else
    {
      strLastURL = url;
      iRestart_cnt = 1;
    }

    // MAX_COUNT_RESTART = 5
    if (iRestart_cnt > MAX_COUNT_RESTART || bytesRead < 1 )
    {
      Logger::Log(LogLevel::LEVEL_INFO, "%s - [%s] stream stalled count: %d", __FUNCTION__, name.c_str(), iRestart_cnt);

      // restart stream
      CloseLiveStream();
      Logger::Log(LogLevel::LEVEL_INFO, "OpenLiveStream - [%s] restart channel: [%s]", name.c_str(), WebUtils::RedactUrl(url).c_str());
      m_streamHandle.CURLCreate(url.c_str());
      if (iCurl_timeout > 0)
        m_streamHandle.CURLAddOption(ADDON_CURL_OPTION_PROTOCOL, "connection-timeout", std::to_string(iCurl_timeout).c_str());

      // ADDON_READ_TRUNCATED | ADDON_READ_CHUNKED | ADDON_READ_NO_CACHE
      if (!m_streamHandle.CURLOpen(ADDON_READ_TRUNCATED | ADDON_READ_CHUNKED | ADDON_READ_NO_CACHE))
      {
        Logger::Log(LogLevel::LEVEL_ERROR, "OpenLiveStream - Could not open streaming for channel [%s]", name.c_str());
        return -1;
      }
      bytesRead = m_streamHandle.Read(pBuffer, iBufferSize);
    }
  }
  else
  {
    iRestart_cnt = 0;
  }
  return bytesRead;
}

bool PVRIptvData::CanPauseStream()
{
  return m_streamHandle.IsOpen();
}

ADDONCREATOR(PVRIptvData)
