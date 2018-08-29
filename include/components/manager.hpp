#pragma once

#include <functional>
#include <memory>
#include <shared_mutex>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include <json/value.h>

#include <engine/coro/pool_stats.hpp>
#include <engine/ev/thread_pool.hpp>
#include <engine/task/task_processor.hpp>

#include "component_base.hpp"
#include "component_config.hpp"
#include "component_context.hpp"
#include "manager_config.hpp"

namespace components {

class ComponentList;
class MonitorableComponentBase;

enum class MonitorVerbosity { kTerse, kFull };

class LockedMonitorableComponentSet {
 public:
  LockedMonitorableComponentSet(
      std::unordered_map<std::string, const MonitorableComponentBase*>
          components,
      std::shared_lock<std::shared_timed_mutex>&& lock)
      : components_(std::move(components)), lock_(std::move(lock)) {}

  auto begin() { return components_.begin(); }
  auto end() { return components_.end(); }

 private:
  const std::unordered_map<std::string, const MonitorableComponentBase*>
      components_;
  std::shared_lock<std::shared_timed_mutex> lock_;
};

class Manager {
 public:
  Manager(ManagerConfig config, const ComponentList& component_list);
  ~Manager();

  const ManagerConfig& GetConfig() const;
  const engine::TaskProcessor::CoroPool& GetCoroPool() const;
  Json::Value GetMonitorData(MonitorVerbosity verbosity) const;
  LockedMonitorableComponentSet GetMonitorableComponentSet() const;

  template <typename Component>
  std::enable_if_t<std::is_base_of<components::ComponentBase, Component>::value>
  AddComponent(const components::ComponentConfigMap& config_map,
               const std::string& name) {
    AddComponentImpl(config_map, name,
                     [](const components::ComponentConfig& config,
                        const components::ComponentContext& context) {
                       return std::make_unique<Component>(config, context);
                     });
  }

  void OnLogRotate();

 private:
  void AddComponents(const ComponentList& component_list);
  void AddComponentImpl(
      const components::ComponentConfigMap& config_map, const std::string& name,
      std::function<std::unique_ptr<components::ComponentBase>(
          const components::ComponentConfig&,
          const components::ComponentContext&)>
          factory);
  void ClearComponents();

  const ManagerConfig config_;

  std::unique_ptr<engine::TaskProcessor::CoroPool> coro_pool_;
  std::unique_ptr<engine::ev::ThreadPool> event_thread_pool_;

  mutable std::shared_timed_mutex context_mutex_;
  std::unique_ptr<components::ComponentContext> component_context_;
  std::unordered_map<std::string, const MonitorableComponentBase*>
      monitorable_components_;
  bool components_cleared_;

  engine::TaskProcessor* default_task_processor_;
};

}  // namespace components
