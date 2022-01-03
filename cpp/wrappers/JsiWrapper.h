#pragma once

#include <map>
#include <mutex>
#include <vector>

#include <jsi/jsi.h>

namespace RNWorklet {

using namespace facebook;

enum JsiWrapperType {
  Undefined,
  Null,
  Bool,
  Number,
  String,
  Array,
  Object,
  HostObject,
  HostFunction,
  Worklet
};

class JsiWrapper {
public:
  /**
   * Constructor - called from static members
   * @param runtime Calling runtime
   * @param value Value to wrap
   * @param parent Optional parent wrapper
   */
  JsiWrapper(jsi::Runtime &runtime, const jsi::Value &value, JsiWrapper *parent)
      : JsiWrapper(parent) {}

  /**
   * Returns a wrapper for the a jsi value
   * @param runtime Runtime to wrap value in
   * @param value Value to wrap
   * @return A new JsiWrapper
   */
  static std::shared_ptr<JsiWrapper> wrap(jsi::Runtime &runtime,
                                          const jsi::Value &value) {
    return JsiWrapper::wrap(runtime, value, nullptr);
  }

  /**
   * Returns the value as a javascript value on the provided runtime
   * @param runtime Runtime
   * @param wrapper Wrapper to get value for
   * @return A new js value in the provided runtime with the wrapped value
   */
  static jsi::Value unwrap(jsi::Runtime &runtime,
                           std::shared_ptr<JsiWrapper> wrapper) {
    return wrapper->getValue(runtime);
  }

  /**
   * Updates the value from a JS value
   * @param runtime runtime for the value
   * @param value Value to set
   */
  virtual void updateValue(jsi::Runtime &runtime, const jsi::Value &value);

  /**
   * @return The type of wrapper
   */
  JsiWrapperType getType() { return _type; }

  /**
   * Returns the object as a string
   */
  virtual std::string toString(jsi::Runtime &runtime);

  /**
   * Add listener
   * @param listener callback to notify
   * @return id of the listener - used for removing the listener
   */
  size_t addListener(std::shared_ptr<std::function<void()>> listener) {
    auto id = _listenerId++;
    _listeners.emplace(id, listener);
    return id;
  }

  /**
   * Remove listener
   * @param listenerId id of listener to remove
   */
  void removeListener(size_t listenerId) { _listeners.erase(listenerId); }

  /**
   * Notify listeners that the value has changed
   */
  void notifyListeners() {
    for (auto listener : _listeners) {
      (*listener.second)();
    }
  }

protected:
  /**
   * Returns a wrapper for the value
   * @param runtime Runtime to wrap value in
   * @param value Value to wrap
   * @param parent Parent wrapper (for nested hierarchies)
   * @return A new JsiWrapper
   */
  static std::shared_ptr<JsiWrapper>
  wrap(jsi::Runtime &runtime, const jsi::Value &value, JsiWrapper *parent);

  /**
   * Call to notify parent that something has changed
   */
  void notifyParent() {
    if (_parent != nullptr) {
      _parent->notifyParent();
    }
    notifyListeners();
  }

  /**
   * Update the type
   * @param type Type to set
   */
  void setType(JsiWrapperType type) { _type = type; }

  /**
   * @return The parent object
   */
  JsiWrapper *getParent() { return _parent; }

private:
  /**
   * Base Constructor
   * @param parent Parent wrapper
   */
  JsiWrapper(JsiWrapper *parent) : _parent(parent) {
    _readWriteMutex = new std::mutex();
  }

  /**
   * Sets the value from a JS value
   * @param runtime runtime for the value
   * @param value Value to set
   */
  virtual void setValue(jsi::Runtime &runtime, const jsi::Value &value);

  /**
   * Returns the value as a javascript value on the provided runtime
   * @param runtime Runtime to set value in
   * @return A new js value in the provided runtime with the wrapped value
   */
  virtual jsi::Value getValue(jsi::Runtime &runtime);

  std::mutex *_readWriteMutex;
  JsiWrapper *_parent;

  JsiWrapperType _type;

  bool _boolValue;
  double _numberValue;
  std::string _stringValue;

  size_t _listenerId = 1000;
  std::map<size_t, std::shared_ptr<std::function<void()>>> _listeners;
};

} // namespace RNWorklet