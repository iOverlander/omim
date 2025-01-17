#pragma once

#include "tracking/archival_manager.hpp"
#include "tracking/archive.hpp"

#include "traffic/speed_groups.hpp"

#include "routing/router.hpp"

#include "platform/location.hpp"

#include "base/thread.hpp"

#include <array>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <string>

namespace tracking
{
// Archive template instances.
using Archive = BasicArchive<Packet>;
using ArchiveCar = BasicArchive<PacketCar>;

class ArchivalReporter
{
public:
  explicit ArchivalReporter(std::string const & host);
  ~ArchivalReporter();

  ArchivalReporter(ArchivalReporter const &) = delete;
  ArchivalReporter & operator=(ArchivalReporter const &) = delete;

  void Insert(routing::RouterType const & trackType, location::GpsInfo const & info,
              traffic::SpeedGroup const & speedGroup);
  void DumpToDisk(bool dumpAnyway = false);

private:
  void Run();

  Archive m_archiveBicycle;
  Archive m_archivePedestrian;
  ArchiveCar m_archiveCar;
  ArchivalManager m_manager;

  bool m_isAlive;
  std::mutex m_mutexRun;
  std::condition_variable m_cvDump;

  threads::SimpleThread m_threadDump;
};
}  // namespace tracking
